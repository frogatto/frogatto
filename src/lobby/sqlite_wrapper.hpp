#pragma once

#ifdef _MSC_VER
#include "windows.h"
#endif

#include <cstdint>
#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <json_spirit.h>


#include "asserts.hpp"
#include "sqlite3.h"

namespace sqlite
{
	struct close_functor
	{
		void operator()(sqlite3* db)
		{
			if(db) {
				sqlite3_close(db);
			}
		}
	};

	struct statement_finalise
	{
		void operator()(sqlite3_stmt* stmt)
		{
			if(stmt) {
				sqlite3_finalize(stmt);
			}
		}
	};

	struct map_value_less
	{
		bool operator()(const json_spirit::mValue& lhs, const json_spirit::mValue& rhs)
		{
			return (&lhs < &rhs);
		}
	};

	typedef std::map<json_spirit::mValue, json_spirit::mValue, map_value_less> bindings_type;
	typedef std::vector<json_spirit::mValue> rows_type;

	class wrapper
	{
	public:
		wrapper(const std::string& db_file_name)
		{
#ifdef BOOST_NO_CXX11_NULLPTR
			sqlite3* db = NULL;
#else
			sqlite3* db = nullptr;
#endif
			int rc = sqlite3_open(db_file_name.c_str(), &db);
			if(rc) {
				sqlite3_close(db);
				ASSERT_LOG(false, "Failed to open database(" << db_file_name << "): " << sqlite3_errmsg(db));
			} else {
				db_ptr_ = boost::shared_ptr<sqlite3>(db, close_functor());
			}
		}

		bool exec(const std::string& sql, 
			const bindings_type& bindings, 
			rows_type* rows)
		{
			boost::mutex::scoped_lock lock(guard_);
			auto it = stmt_ptr_map_.find(sql);
			sqlite3_stmt* stmt;
			if(it == stmt_ptr_map_.end()) {
				sqlite3_stmt* new_stmt;
				int ret = sqlite3_prepare_v2(db_ptr_.get(), sql.c_str(), sql.size(), &new_stmt, NULL);
				if(new_stmt == NULL ) {
					std::cerr << "Failed to execute statement: " << sql << std::endl;
					return false;
				}
				if(ret != SQLITE_OK) {
					std::cerr << "Failed to execute statement: " << sql << ", " << ret << std::endl;
					sqlite3_finalize(new_stmt);
					return false;
				}
				stmt_ptr_map_[sql] = boost::shared_ptr<sqlite3_stmt>(new_stmt, statement_finalise());
				stmt = new_stmt;
			} else {
				stmt = it->second.get();
				// Reset the prepared statement, which is more efficient than re-creating
				// as preparing a statement is expensive.
				sqlite3_reset(stmt);
				sqlite3_clear_bindings(stmt);
			}
			for(auto bit : bindings) {
				int ndx = 0;
				if(bit.first.type() == json_spirit::int_type) {
					ndx = bit.first.get_int();
				} else if(bit.first.type() == json_spirit::str_type) {
					ndx = sqlite3_bind_parameter_index(stmt, bit.first.get_str().c_str());
				} else {
					ASSERT_LOG(false, "parameter for binding index must be int or string: " << bit.first.type());
				}
				ASSERT_LOG(ndx != 0, "Bad index value: 0");
				switch(bit.second.type()) {
					case json_spirit::obj_type: {
						json_spirit::mObject& obj = bit.second.get_obj();
						std::string s = json_spirit::write(obj);
						sqlite3_bind_blob(stmt, ndx, static_cast<const void*>(s.c_str()), s.size(), SQLITE_TRANSIENT);
						break;
					}
					case json_spirit::array_type: {
						json_spirit::mArray& ary = bit.second.get_array();
						std::string s = json_spirit::write(ary);
						sqlite3_bind_blob(stmt, ndx, static_cast<const void*>(s.c_str()), s.size(), SQLITE_TRANSIENT);
						break;
					}
					case json_spirit::str_type: {
						std::string s = bit.second.get_str();
						sqlite3_bind_text(stmt, ndx, s.c_str(), s.size(), SQLITE_TRANSIENT);
						break;
					}
					case json_spirit::bool_type: {
						sqlite3_bind_int(stmt, ndx, bit.second.get_bool());
						break;
					}
					case json_spirit::int_type: {
						sqlite3_bind_int(stmt, ndx, bit.second.get_int());
						break;
					}
					case json_spirit::real_type: {
						sqlite3_bind_double(stmt, ndx, bit.second.get_real());
						break;
					}
					case json_spirit::null_type: {
						sqlite3_bind_null(stmt, ndx);
						break;
					}
				}
			}
			
			std::vector<json_spirit::mValue> ret_rows;
			// Step through the statement
			bool stepping = true;
			while(stepping) {
				int ret = sqlite3_step(stmt);
				if(ret == SQLITE_ROW) {
					int col_count = sqlite3_column_count(stmt);
					for(int n = 0; n != col_count; ++n) {
						int type = sqlite3_column_type(stmt, n);
						switch(type) {
							case SQLITE_INTEGER: {
								int val = sqlite3_column_int(stmt, n);
								ret_rows.push_back(json_spirit::mValue(val));
								break;
							}
							case SQLITE_FLOAT: {
								double val = sqlite3_column_double(stmt, n);
								ret_rows.push_back(json_spirit::mValue(val));
								break;
							}
							case SQLITE_BLOB: {
								const char* blob = reinterpret_cast<const char *>(sqlite3_column_blob(stmt, n));
								int len = sqlite3_column_bytes(stmt, n);
								json_spirit::mValue value;
								json_spirit::read(std::string(blob, blob + len), value);
								ret_rows.push_back(value);
								break;
							}
							case SQLITE_NULL: {
								ret_rows.push_back(json_spirit::mValue());
								break;
							}
							case SQLITE_TEXT: {
								const uint8_t* us = sqlite3_column_text(stmt, n);
								int len = sqlite3_column_bytes(stmt, n);
								std::string s(us, us + len);
								ret_rows.push_back(json_spirit::mValue(s));
								break;
							}
						}						
					}
				} else if(ret == SQLITE_DONE) {
					stepping = false;
				} else {
					std::cerr << "Error stepping through statement: " << sql << ", " << ret << " : " << sqlite3_errmsg(db_ptr_.get()) << std::endl;
					return false;
				}
			}

			ASSERT_LOG(rows != NULL || ret_rows.empty() != false, "There was data to return but no place to put it.");
			rows->swap(ret_rows);
			return true;
		}
	private:
		boost::shared_ptr<sqlite3> db_ptr_;
		std::map<std::string, boost::shared_ptr<sqlite3_stmt> > stmt_ptr_map_;
		mutable boost::mutex guard_;
	};

	typedef boost::shared_ptr<wrapper> sqlite_wrapper_ptr;
}