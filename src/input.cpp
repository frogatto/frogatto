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
                
#if defined(__ANDROID__) && !SDL_VERSION_ATLEAST(2, 0, 0)
            case SDL_VIDEORESIZE: 
                // Allow restore from app going to the background on android, while a modal dialog is up.
                video_resize( event ); 
                break;
#endif

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE || (defined(__ANDROID__) && SDL_VERSION_ATLEAST(2, 0, 0))
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

}

