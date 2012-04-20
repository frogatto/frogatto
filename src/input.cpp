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

    Uint8 get_button_change_state(Uint8 button) {
        return (1 << (button-1));
    }

    SDLMod get_mod_change_state(const SDLKey& key) {
        SDLMod ret;
#if SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION == 3
#define SDLK_LMETA SDLK_LGUI
#define SDLK_RMETA SDLK_RGUI
#endif
        switch(key) {
        case SDLK_LSHIFT:
            ret = KMOD_LSHIFT;
            break;
        case SDLK_RSHIFT:
            ret = KMOD_RSHIFT;
            break;
        case SDLK_LCTRL:
            ret = KMOD_LCTRL;
            break;
        case SDLK_RCTRL:
            ret = KMOD_RCTRL;
            break;
        case SDLK_LALT:
            ret = KMOD_LALT;
            break;
        case SDLK_RALT:
            ret = KMOD_RALT;
            break;
        case SDLK_LMETA:
            ret = KMOD_LMETA;
            break;
        case SDLK_RMETA:
            ret = KMOD_RMETA;
            break;
        case SDLK_NUMLOCK:
            ret = KMOD_NUM;
            break;
        case SDLK_CAPSLOCK:
            ret = KMOD_CAPS;
            break;
        case SDLK_MODE:
            ret = KMOD_MODE;
            break;
        default:
            ret = KMOD_NONE;
            break;
        }

        return ret;
    }
        
    void key_listener::expand_keys(int logical_key, const SDL_keysym& sym,
                                   const SDLMod& processed_mask, int depth) {
        SDL_keysym tmp;
        tmp = sym;

        static const int NUM_CHUNKS = 4;
        static const SDLMod chunks[NUM_CHUNKS] = { 
            (SDLMod)KMOD_SHIFT, (SDLMod)KMOD_ALT, 
            (SDLMod)KMOD_CTRL,  (SDLMod)KMOD_META 
        };
        static const SDLMod ALL_CHUNKS = (SDLMod)(KMOD_SHIFT | KMOD_ALT | KMOD_CTRL | KMOD_META);

        static const int NUM_BITS = sizeof(SDLMod)*8;
        int bit_idx[NUM_BITS];

        int remaining = ALL_CHUNKS & sym.mod & ~processed_mask;
        if(remaining == 0) {
            bind_key(logical_key, sym, false);
            return;
        } 

        for(int i =0;i<NUM_CHUNKS;++i) {
            if(sym.mod & chunks[i] && !(processed_mask & chunks[i])) {
                SDLMod newmask = (SDLMod)(processed_mask | chunks[i]);
                SDLMod newmod = (SDLMod)(sym.mod & ~chunks[i]);

                int bit_count = 0;
                for(int j=0;j<NUM_BITS;++j) {
                    if(chunks[i] & (1 << j)) {
                        bit_idx[bit_count++] = j;
                    }
                }
                int k_max = 1 << bit_count;
                for(int k=1;k < k_max;++k) {
                    tmp.mod = newmod;
                    for(int j=0;j<bit_count;++j) {
                        int bit = 1 << j;
                        if(k & bit) {
                            tmp.mod = (SDLMod)(tmp.mod | (1 << bit_idx[j]));
                        }
                    }
                    expand_keys(logical_key, tmp, newmask, depth+1);
                }
                return;
            }
        }
    }        

    void key_listener::bind_key(int logical_key, const SDL_keysym& sym,
                                bool collapse_doubles) {
        if(collapse_doubles) {
            expand_keys(logical_key, sym, KMOD_NONE,0);
            return;
        }

        unbind_key(sym);

        binding_map::iterator binding_itor = bindings_.find(sym.sym);
        if(binding_itor == bindings_.end()) {
            mod_map m;
            bindings_[sym.sym] = m;
            binding_itor = bindings_.find(sym.sym);
        }
        assert(binding_itor != bindings_.end());
        binding_itor->second[static_cast<SDLMod>(sym.mod)] = logical_key;
    }
    
    void key_listener::bind_key(int logical_key, const SDLKey& k, 
                                const SDLMod& m, bool collapse_doubles) {
        SDL_keysym sym;
        sym.sym = k;
        sym.mod = m;
        bind_key(logical_key, sym, collapse_doubles);
    }
    
    bool key_listener::unbind_key(const SDL_keysym& k) {
        bool had_key = false;

        binding_map::iterator binding_itor = bindings_.find(k.sym);
        if(binding_itor != bindings_.end()) {
            mod_map& mm = binding_itor->second;
            mod_map::iterator mod_itor = mm.find(static_cast<SDLMod>(k.mod));
            if(mod_itor != mm.end()) {
                mm.erase(mod_itor);
                had_key = true;
            }
        }
        clean_mod_maps();
        
        return had_key;
    }
    
    /* find every instance of logical_key in the nested maps
       and erase it, erasing any mod_maps that become empty*/
    bool key_listener::unbind_key(int logical_key) {
        bool had_key = false;
        bool erased;
        do {
            erased = false;
            for(binding_map::iterator binding_itor = bindings_.begin();
                binding_itor != bindings_.end();
                ++binding_itor) {
                mod_map& mm = binding_itor->second;
                for(mod_map::iterator mod_itor = mm.begin();
                    mod_itor != mm.end();
                    ++mod_itor) {

                    if(mod_itor->second == logical_key) {
                        mm.erase(mod_itor);
                        had_key = true;
                        erased = true;
                        break;
                    }
                }
                if(erased) {
                    break;
                }
            }
        } while(erased);

        clean_mod_maps();

        return had_key;
    }

    void key_listener::clean_mod_maps() {
        bool erased;
        do {
            erased = false;
            for(binding_map::iterator binding_itor = bindings_.begin();
                binding_itor != bindings_.end();
                ++binding_itor) {
                if(binding_itor->second.empty()) {
                    bindings_.erase(binding_itor);
                    erased = true;
                    break;
                }
            }
        } while(erased);
    }
    
    int key_listener::bound_key(const SDL_keysym& sym) const {
        binding_map::const_iterator binding_itor;
        binding_itor = bindings_.find(sym.sym);
        if(binding_itor == bindings_.end()) {
            return -1;
        }
        mod_map::const_iterator mod_itor;
        const mod_map& mm = binding_itor->second;
        mod_itor = mm.find(static_cast<SDLMod>(sym.mod));
        if(mod_itor == mm.end()) {
            return -1;
        }
        return mod_itor->second;
    }

    bool key_listener::check_keys(const SDL_keysym& sym, Uint32 type) {
        binding_map::iterator binding_itor;
        binding_itor = bindings_.find(sym.sym);
        
        bool changed = false;

        switch(type) {
        case SDL_KEYDOWN:
            if(binding_itor != bindings_.end()) {
                mod_map::iterator mod_itor = binding_itor->second.find(static_cast<SDLMod>(sym.mod));
                if(mod_itor != binding_itor->second.end()) {
                    do_keydown(mod_itor->second);
                    changed = true;
                }
            }
            break;
        case SDL_KEYUP:
            if(binding_itor != bindings_.end()) {
                for(mod_map::iterator mod_itor = binding_itor->second.begin();
                    mod_itor != binding_itor->second.end();
                    ++mod_itor) {
                    do_keyup(mod_itor->second);
                }
                changed = true;
            }
            break;
        default:
            break;
       }

        return changed;
    }
    
    bool key_listener::process_event(const SDL_Event& event, bool claimed) {
        switch(event.type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            if(claimed) {
                reset();
                break;
            }
            if(check_keys(event.key.keysym, event.type)) {
                claimed = true;
            }
            break;
        default:
            break;
        }
        
        return claimed;
    }

    void key_down_listener::bind_key(int logical_key, const SDL_keysym& sym,
                                     bool collapse_doubles) {
        key_listener::bind_key(logical_key, sym, collapse_doubles);
        if(logical_key >= state_.size()) {
            state_.resize(logical_key+1);
        }
        state_[logical_key] = false;
    }
    
    bool key_down_listener::unbind_key(const SDL_keysym& k) {
        int key = bound_key(k);
        if(key >= 0 && key < state_.size()) {
            state_[key] = false;
        }
        return key_listener::unbind_key(k);
    }
    
    bool key_down_listener::unbind_key(int logical_key) {
        if(logical_key < state_.size()) {
            state_[logical_key] = false;
        }
        return key_listener::unbind_key(logical_key);
    }

    void key_down_listener::do_keydown(int key) {
        state_[key] = true;
    }
    void key_down_listener::do_keyup(int key) {
        state_[key] = false;
    }

    void key_down_listener::reset() {
        std::vector<bool>::iterator itor;
        for(itor = state_.begin(); itor != state_.end(); ++itor) {
            *itor = false;
        }
    }

#ifndef NO_EDITOR
    bool mouse_drag_listener::process_event(const SDL_Event& event, bool claimed) {
        Sint32 pos[2];
        Sint16 rel[2];
        Uint8 state;
        Uint8 button_change_state;
        SDLMod mod;
        SDLMod mod_change_state;
        
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
            mod_change_state = (SDLMod)
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

