#include <boost/shared_ptr.hpp>
#include <iostream>

#include "input.hpp"
#include "scoped_resource.hpp"

#include "userevents.h"
#include "level_runner.hpp"

namespace input {
    pump::~pump() {
        SDL_Event e;
        
        e.type = SDL_USEREVENT;
        e.user.code = ST_EVENT_NESTED_DEATH;
        e.user.data1 = NULL;
        e.user.data2 = NULL;
        SDL_PushEvent(&e);
    }

    bool pump::process() {
        SDL_Event event;
        while(!killed_ && SDL_PollEvent(&event)) {  
            bool claimed = false;
            
            switch(event.type) {
            case SDL_QUIT:
                killed_ = true;
                claimed = true;
                SDL_PushEvent(&event);
                break;
                
#if defined(__ANDROID__)
            case SDL_VIDEORESIZE: 
                // Allow restore from app going to the background on android, while a modal dialog is up.
                video_resize( event ); 
                break;
#endif

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
				case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_MINIMIZED)
				{
					SDL_Event e;
					while (SDL_WaitEvent(&e))
					{
						if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESTORED)
							break;
					}
				}
				break;
#endif
            case SDL_USEREVENT:
                if(event.user.code == ST_EVENT_NESTED_DEATH) {
                    reset();
                    continue;
                }
                break;
            default:
                break;
            }
            claimed = process_event(event, claimed);
        }

        return !killed_;
    }

    namespace {
        struct exit_event { void operator()(int* process_stack) const { --(*process_stack); } };
        typedef util::scoped_resource<int*, exit_event> scoped_int_stack;
    }

    bool listener_container::process_event(const SDL_Event& event, bool claimed) {
        {
            scoped_int_stack stack_fixer(&(++process_event_stack_));
            std::vector<listener*>::reverse_iterator raw_itor;
            for(raw_itor = raw_listeners_.rbegin(); raw_itor != raw_listeners_.rend(); ++raw_itor) {
                claimed |= (*raw_itor)->process_event(event, claimed);
            }
        }

        post_process_fixup();

        return claimed;
    }

    void listener_container::reset() {
        std::vector<listener*>::iterator raw_itor;
        for(raw_itor = raw_listeners_.begin(); raw_itor != raw_listeners_.end(); ++raw_itor) {
            (*raw_itor)->reset();
        }
    }

    void listener_container::register_listener(listener_ptr p) {
        listeners_.push_back(p);
        register_listener(p.get());
    }
    void listener_container::deregister_listener(listener_ptr p) {
        deregister_listener(p.get());
        if(!can_change_listeners()) {
            reference_holder_.push_back(p);
        }
        std::vector<listener_ptr>::iterator itor = std::find(listeners_.begin(), listeners_.end(), p);
        if(itor != listeners_.end()) {
            listeners_.erase(itor);
        }
    }
    void listener_container::register_listener(listener *p) {
        if(can_change_listeners()) {
            raw_listeners_.push_back(p);
        } else {
            std::vector<listener*>::iterator itor = std::find(pending_removal_listeners_.begin(),
                                                              pending_removal_listeners_.end(), p);
            if(itor != pending_removal_listeners_.end()) {
                pending_removal_listeners_.erase(itor);
            } else {
                pending_addition_listeners_.push_back(p);
            }
        }
    }
    void listener_container::deregister_listener(listener *p) {
        if(can_change_listeners()) {
            std::vector<listener*>::iterator itor = std::find(raw_listeners_.begin(), raw_listeners_.end(), p);
            if(itor != raw_listeners_.end()) {
                raw_listeners_.erase(itor);
            }
        } else {
            std::vector<listener*>::iterator itor = std::find(pending_addition_listeners_.begin(),
                                                              pending_addition_listeners_.end(), p);
            if(itor != pending_addition_listeners_.end()) {
                pending_addition_listeners_.erase(itor);
            } else {
                pending_removal_listeners_.push_back(p);
            }
        }
    }
    
    void listener_container::post_process_fixup() {
        if(!can_change_listeners()) {
            return;
        }
        if(!pending_removal_listeners_.empty()) {
            std::vector<listener*>::iterator itor, found_itor;
            for(itor = pending_removal_listeners_.begin();
                itor != pending_removal_listeners_.end();
                ++itor) {
                found_itor = std::find(raw_listeners_.begin(), raw_listeners_.end(), *itor);
                if(found_itor != raw_listeners_.end()) {
                    raw_listeners_.erase(found_itor);
                }
            }
            pending_removal_listeners_.clear();
        }
        if(!pending_addition_listeners_.empty()) {
            raw_listeners_.insert(raw_listeners_.end(), 
                                  pending_addition_listeners_.begin(),
                                  pending_addition_listeners_.end());
            pending_addition_listeners_.clear();
        }
        reference_holder_.clear();
    }
       
    void key_listener::expand_keys(int logical_key, const SDL_Scancode& sym,
                                   const SDL_Keymod& processed_mask, int depth) {
    }        

    void key_listener::bind_key(int logical_key, const SDL_Scancode& sym,
                                bool collapse_doubles) {
    }
    
    void key_listener::bind_key(int logical_key, const SDL_Keycode& k, 
                                const SDL_Keymod& m, bool collapse_doubles) {
    }
    
    bool key_listener::unbind_key(const SDL_Scancode& k) {
		return false;
    }
    
    bool key_listener::unbind_key(int logical_key) {
		return false;
    }

    void key_listener::clean_mod_maps() {
    }
    
    int key_listener::bound_key(const SDL_Scancode& sym) const {
		return 0;
    }

    bool key_listener::check_keys(const SDL_Scancode& sym, Uint32 type) {
		return 0;
    }
    
    bool key_listener::process_event(const SDL_Event& event, bool claimed) {
		return 0;
    }

    void key_down_listener::bind_key(int logical_key, const SDL_Scancode& sym,
                                     bool collapse_doubles) {
    }
    
    bool key_down_listener::unbind_key(const SDL_Scancode& k) {
    }
    
    bool key_down_listener::unbind_key(int logical_key) {
    }

    void key_down_listener::do_keydown(int key) {
    }
    void key_down_listener::do_keyup(int key) {
    }

    void key_down_listener::reset() {
    }

#ifndef NO_EDITOR
    bool mouse_drag_listener::process_event(const SDL_Event& event, bool claimed) {
        Sint32 pos[2];
        Sint16 rel[2];
        Uint8 state;
        Uint8 button_change_state;
        SDL_Keymod mod;
        SDL_Keymod mod_change_state;
        
        bool was_capturing = is_capturing_;
        
        if(claimed) {
            is_capturing_ = false;
            return claimed;
        }
        
        switch(event.type) {
        case SDL_MOUSEMOTION:
            pos[0] = event.motion.x;
            pos[1] = event.motion.y;
            rel[0] = event.motion.xrel;
            rel[1] = event.motion.yrel;
            state = event.motion.state;
            mod = SDL_GetModState();
            button_change_state = 0;
            mod_change_state = KMOD_NONE;
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            pos[0] = event.button.x;
            pos[1] = event.button.y;
            rel[0] = 0;
            rel[1] = 0;
            state = SDL_GetMouseState(NULL,NULL);
            mod = SDL_GetModState();
            button_change_state = 
                get_button_change_state(event.button.button) 
                & target_state_mask();
            mod_change_state = KMOD_NONE;
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            state = SDL_GetMouseState(&(pos[0]), &(pos[1]));
            rel[0] = 0;
            rel[1] = 0;
            mod = event.key.keysym.mod;
            button_change_state = 0;
            mod_change_state = (SDL_Keymod)
                (get_mod_change_state(event.key.keysym.sym)
                 & target_mod_mask());
            break;
        default:
            return claimed;
        }
        
        if(!matches_target_state(state)) {
            is_capturing_ = false;
            return claimed;
        } else if(!matches_target_mod(mod)) {
            is_capturing_ = false;
            return claimed;
        } 
        
        if(!is_capturing_) {
            /* to start a gesture, we must be in the target area (if one is defined) 
               AND this must be a mouse state change event if buttons are involved 
               OR this must be a key state change event if keys are involved
            */
            if(target_state() != 0) {
                /* if we have a target state, this must have been an event
                   which indicated an unmasked button was pressed */
                if(button_change_state == 0) {
                    return claimed;
                }
            } else if(target_mod() != 0) {
                /* if we don't have a target state, but do have target modifiers,
                   this must have been an event that indicated an unmasked
                   modifier was pressed */
                if(mod_change_state == 0) {
                    return claimed;
                }
            }
            if(target_area_enabled() && !in_target_area(pos[0], pos[1])) {
                return claimed;
            }
        }

        is_capturing_ = true;
        claimed = true;
        
        pos_[0] = pos[0];
        pos_[1] = pos[1];
        state_ = state;
        mod_ = mod;
        
        rel_[0] = rel[0];
        rel_[1] = rel[1];

        if(was_capturing) {
            total_rel_[0] += rel[0];
            total_rel_[1] += rel[1];
        } else {
            total_rel_[0] = rel[0];
            total_rel_[1] = rel[1];
            start_pos_[0] = pos[0];
            start_pos_[1] = pos[1];
        }
        
        do_drag();

        return claimed;
    }

    bool mouse_click_listener_base::process_event(const SDL_Event& event, bool claimed) {
        if(claimed) {
            click_count_ = 0;
            clicked_ = false;
            return claimed;
        }

        switch(event.type) {
        case SDL_MOUSEMOTION:
            pos_[0] = event.motion.x;
            pos_[1] = event.motion.y;
            state_ = event.motion.state;
            button_state_ = 0;
            mod_ = SDL_GetModState();
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            pos_[0] = event.button.x;
            pos_[1] = event.button.y;
            state_ = SDL_GetMouseState(NULL,NULL);
            mod_ = SDL_GetModState();
            button_state_ = get_button_change_state(event.button.button);
            if(event.type == SDL_MOUSEBUTTONDOWN) {
                state_ |= button_state_;
            }
            break;
        default:
            return claimed;
        }
        
        /* this handles exiting from a click */
        if(!matches_target_state(state_)) {
            if(clicked_) {
                click_time_ = SDL_GetTicks();
                clicked_ = false;
                do_click(pos_[0], pos_[1], click_count_, state_, button_state_, mod_);
            }
            return claimed;
        }

        switch(event.type) {
        case SDL_MOUSEMOTION:
            /* next continuing in a click - claim motion events
               whilst clicking */
            if(clicked_) {
                claimed = true;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            /* finally beginning a click */
            if(!clicked_ && matches_target_mod(mod_)) {
                /* to start a click, we must be in the target area */
                if(target_area_enabled() && !in_target_area(pos_[0], pos_[1])) {
                    break;
                }
                clicked_ = true;
                Uint32 curtime = SDL_GetTicks();
                if(click_count_ > 0 && 
                   curtime - click_time_ >= click_timeout_) {
                    click_count_ = 0;
                }
                click_count_++;
                claimed = true;
            }
            break;
        default:
            break;
        }
        
        return claimed;
    }
#endif // NO_EDITOR

}

