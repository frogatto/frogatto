/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef INPUT_HPP_INCLUDED
#define INPUT_HPP_INCLUDED

#include "formula_callable.hpp"
#include "graphics.hpp"
#include <boost/intrusive_ptr.hpp>
#include <vector>
#include <map>

namespace input {

    class listener : public virtual game_logic::formula_callable {
    public:
        virtual ~listener() {}
        /** returns true if the event is now claimed */
        virtual bool process_event(const SDL_Event &e, bool is_claimed) =0;
		virtual void reset() {};
	protected:
		virtual variant get_value(const std::string& key) const {return variant();}
    };

    typedef boost::intrusive_ptr<listener> listener_ptr;

    class listener_container: public virtual listener {
    public:
        listener_container() : process_event_stack_(0) {}
        bool process_event(const SDL_Event &e, bool is_claimed);
        virtual void reset();
        void register_listener(listener_ptr p);
        void register_listener(listener *p);
        void deregister_listener(listener_ptr p);
        void deregister_listener(listener *p);
    private:
        bool can_change_listeners() { return process_event_stack_ == 0; }
        void post_process_fixup();
        int process_event_stack_;
        std::vector<listener_ptr> listeners_;
        std::vector<listener*> raw_listeners_;
        std::vector<listener*> pending_addition_listeners_, pending_removal_listeners_;
        std::vector<listener_ptr> reference_holder_;
    };

    typedef boost::intrusive_ptr<listener_container> listener_container_ptr;

    class delegate_listener: public virtual listener {
    public:
        delegate_listener(listener *delegate) : delegate_(delegate) {}
        bool process_event(const SDL_Event &e, bool claimed) {
            return delegate_->process_event(e, claimed);
        }
        void reset() { delegate_->reset(); }
    private:
        listener* delegate_;
    };

    class pump: private listener_container {
    public:
        pump() : killed_(false) {}
        virtual ~pump();

        using listener_container::reset;
        using listener_container::register_listener;
        using listener_container::deregister_listener;

        bool process();
        void resurrect() {
            killed_ = false;
        }
        bool killed() const {
            return killed_;
        }
    private:
        bool killed_;
    };
}

#endif
