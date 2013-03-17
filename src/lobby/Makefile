OPTIMIZE=yes
CCACHE?=ccache
USE_CCACHE?=$(shell which $(CCACHE) 2>&1 > /dev/null && echo yes)
ifneq ($(USE_CCACHE),yes)
CCACHE=
endif

ifeq ($(OPTIMIZE),yes)
BASE_CXXFLAGS += -O2
endif

# Initial compiler options, used before CXXFLAGS and CPPFLAGS.
# -std=c++11
BASE_CXXFLAGS += -g -Werror -std=gnu++0x -I/usr/local/include -DUSE_SQLITE
BASE_CCFLAGS += -g -Werror -DSQLITE_THREADSAFE=1
BASE_LDFLAGS += -L/usr/local/lib -lboost_regex -lboost_system -lboost_chrono -lboost_thread -ljson_spirit -lpthread -ldl -fthreadsafe-statics

objects = \
	client.o \
	connection.o \
	game_server_worker.o \
	lobby.o \
	mime_types.o \
	name.o \
	reply.o \
	request_handler.o \
	request_parser.o \
	server.o \
	shared_data.o

lobby: $(objects)
	@echo "Linking: lobby ..."
	@$(CCACHE) $(CXX) $(BASE_CXXFLAGS) $(LDFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(objects) $(BASE_LDFLAGS) -o lobby
	@echo "Done"

%.o : %.cpp
	@echo "Building: " $<
	@$(CCACHE) $(CXX) $(BASE_CXXFLAGS) $(CXXFLAGS) $(CPPFLAGS) -c $<
	@$(CXX) $(BASE_CXXFLAGS) $(CXXFLAGS) $(CPPFLAGS) -MM $< > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
		sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

sqlite3.o: sqlite3.c
	@echo "Building: " $<
	@$(CC) $(BASE_CCFLAGS) $(CCFLAGS) -c $<
	@$(CC) $(BASE_CCFLAGS) $(CCFLAGS) -MM $< > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
		sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

# pull in dependency info for *existing* .o files
-include $(objects:.o=.d)

clean:
	rm -f *.o *.d backend
