#ifndef INPUT_HPP_INCLUDED
#define INPUT_HPP_INCLUDED

#include "graphics.hpp"
#include <boost/shared_ptr.hpp>
#include <vector>
#include <map>

namespace input {

    class listener {
    public:
        virtual ~listener() {}
        /** returns true if the event is now claimed */
        virtual bool process_event(const SDL_Event &e, bool is_claimed) =0;
        virtual void reset() {}
    };

    typedef boost::shared_ptr<listener> listener_ptr;

    class listener_container: public virtual listener {
    public:
        listener_container() : process_event_stack_(0) {}
        bool process_event(const SDL_Event &e, bool is_claimed);
        void reset();
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

    typedef boost::shared_ptr<listener_container> listener_container_ptr;

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

    SDLMod get_mod_change_state(const SDLKey& k);
    Uint8 get_button_change_state(Uint8 button);

    class key_listener: public listener {
    public:
        virtual void bind_key(int logical_key, const SDL_keysym& sym, 
                              bool collapse_doubles = true);
        virtual void bind_key(int logical_key, const SDLKey& k, const SDLMod& m,
                              bool collapse_doubles = true);
        virtual bool unbind_key(int logical_key);
        virtual bool unbind_key(const SDL_keysym& sym);
        int bound_key(const SDL_keysym& sym) const;
        bool process_event(const SDL_Event& event, bool claimed);
    protected:
        bool check_keys(const SDL_keysym& sym, Uint32 event_type);
        virtual void do_keydown(int key) =0;
        virtual void do_keyup(int key) =0;
    private:
        void clean_mod_maps();
        void expand_keys(int logical_key, const SDL_keysym& sym,
                         const SDLMod &mask, int depth);
        
        typedef std::map<SDLMod,int> mod_map;
        typedef std::map<SDLKey,mod_map> binding_map;

        binding_map bindings_;
    };

    class key_down_listener: public key_listener {
    public:
        bool key(int logical_key) const { return state_[logical_key]; }
        void reset();
        void bind_key(int logical_key, const SDL_keysym& sym,
                      bool collapse_doubles = true);
        void bind_key(int logical_key, const SDLKey& k, const SDLMod& m,
                      bool collapse_doubles = true) {
            key_listener::bind_key(logical_key, k, m, collapse_doubles);
        }
        bool unbind_key(int logical_key);
        bool unbind_key(const SDL_keysym& sym);
    protected:
        void do_keydown(int key);
        void do_keyup(int key);
    private:
        std::vector<bool> state_;
    };

    /* T provides T->keydown(key), T->keyup(key) */
    template <class T> class key_press_listener: public key_listener {
    public:
        key_press_listener(T owner) : owner_(owner) {}
        void reset() {}
    protected:
        void do_keydown(int key) {
            owner_->keydown(key);
        }
        void do_keyup(int key) {
            owner_->keyup(key);
        }
    private:
        T owner_;
    };

    /* convenient abstract base */
    class mouse_listener: public listener {
    public:
        static const SDLMod MOD_MASK_NONE = (SDLMod)-1;
        static const SDLMod MOD_MASK_ALL = (SDLMod)0;
        static const Uint8 STATE_MASK_NONE = 0xFF;
        static const Uint8 STATE_MASK_ALL = 0;

        mouse_listener() {
            target_area_.x = 0;
            target_area_.y = 0;
            target_area_.w = 0;
            target_area_.h = 0;
            target_state_ = 0;
            target_state_mask_ = STATE_MASK_ALL;
            target_mod_ = KMOD_NONE;
            target_mod_mask_ = MOD_MASK_ALL;    
        }
        
        void set_target_area(const SDL_Rect& rect) {
            target_area_ = rect;
        }
        SDL_Rect target_area() const { 
            return target_area_; 
        }
        void disable_target_area() {
            target_area_.w = 0;
            target_area_.h = 0;
        }
        bool target_area_enabled() const {
            return target_area_.w != 0 && target_area_.h != 0;
        }
        
        void set_target_mod(SDLMod mod, SDLMod mask) { 
            target_mod_ = mod;
            target_mod_mask_ = (SDLMod)(mask | mod);
        }
        SDLMod target_mod() const {
            return target_mod_;
        }
        SDLMod target_mod_mask() const {
            return target_mod_mask_;
        }
        
        void set_target_state(Uint8 state, Uint8 mask) {
            target_state_ = state;
            target_state_mask_ = mask | state;
        }
        Uint8 target_state() const {
            return target_state_;
        }
        Uint8 target_state_mask() const {
            return target_state_mask_;
        }
    protected:
        bool in_target_area(Sint32 x, Sint32 y) const {
            return x >= target_area_.x &&
                x < target_area_.x + target_area_.w &&
                y >= target_area_.y &&
                y < target_area_.y + target_area_.h;
        }
        
        bool matches_target_state(const Uint8& state) const {
            return (state & target_state_mask_ )== target_state_;
        }
        bool matches_target_mod(const SDLMod& mod) const {
            return (mod & target_mod_mask_) == target_mod_;
        }
    private:
        SDL_Rect target_area_; //= {0,0,0,0};
        Uint8 target_state_;
        Uint8 target_state_mask_;// = STATE_MASK_ALL;
        SDLMod target_mod_;// = 0;
        SDLMod target_mod_mask_;// = MOD_MASK_ALL;    
    };

    class mouse_drag_listener: public mouse_listener {
    public:
        mouse_drag_listener() {
            is_capturing_ = false;
            pos_[0] = 0;
            pos_[1] = 0;
            start_pos_[0] = 0;
            start_pos_[1] = 0;
            rel_[0] = 0;
            rel_[1] = 0;
            total_rel_[0] = 0;
            total_rel_[1] = 0;
            mod_ = KMOD_NONE;
            state_ = 0;
        }
        
        bool process_event(const SDL_Event& event, bool claimed);
        void reset() {
            is_capturing_ = false;
        }
        
        /* whether the state variables refer to an active drag,
           and are being updated, or if the drag is complete -
           in which case they are the last values before the drag
           completed */
        bool active() const {
            return is_capturing_;
        }
        
        /* pos is the last observed mouse drag position */
        Sint32 pos_x() const {
            return pos_[0];
        }
        Sint32 pos_y() const {
            return pos_[1];
        }
        /* the position the drag started at */
        Sint32 start_pos_x() const {
            return start_pos_[0];
        }
        Sint32 start_pos_y() const {
            return start_pos_[1];
        }

        /* rel is the accumulated motion of the mouse 
           over the duration of the drag; not guaranteed by
           SDL to be equal to pos-start_pos */
        Sint16 rel_x() const {
            return rel_[0];
        }
        Sint16 rel_y() const {
            return rel_[1];
        }
        Sint16 total_rel_x() const {
            return total_rel_[0];
        }
        Sint16 total_rel_y() const {
            return total_rel_[1];
        }
        /* the last modifiers observed */
        SDLMod mod() const {
            return mod_;
        }
        /* the last state observed */
        Uint8 state() const {
            return state_;
        }

        void reset_start_pos() {
            start_pos_[0] = pos_[0];
            start_pos_[1] = pos_[1];
        }
    protected:
        virtual void do_drag() {}
    private:
        Sint32 pos_[2];
        Sint32 start_pos_[2];
        Sint16 rel_[2];
        Sint16 total_rel_[2];
        SDLMod mod_;
        Uint8 state_;
        bool is_capturing_;
    };

    template <class T> class active_mouse_drag_listener: public mouse_drag_listener {
    public:
        active_mouse_drag_listener(T owner) : owner_(owner) {}
        void do_drag() {
            owner_->drag();
        }
    private:
        T owner_;
    };

    class mouse_click_listener_base: public mouse_listener {
    public:
        mouse_click_listener_base() {
            click_time_ = 0;
            click_timeout_ = 0;
            click_count_ = 0;
            clicked_ = false;
            state_ = 0;
            button_state_ = 0;
            mod_ = KMOD_NONE;
            pos_[0] = 0;
            pos_[1] = 0;
        }
        bool process_event(const SDL_Event& event, bool claimed);
        void reset() { clicked_ = false; }
        void set_click_timeout(Uint32 amount) { click_timeout_ = amount; }
        Uint32 click_timeout() const { return click_timeout_; }
        virtual void do_click(Sint32 x, Sint32 y, int count, Uint8 state, Uint8 button_state, SDLMod mod)=0;
    private:
        bool clicked_;
        int click_count_;
        Uint32 click_time_;
        Uint32 click_timeout_;
        Uint8 state_, button_state_;
        SDLMod mod_;
        Sint32 pos_[2];
    };

    /* T provides T->click(x,y, clicks, buttons, buttons changed, key modifiers) */
    template <class T> class mouse_click_listener: public mouse_click_listener_base {
    public:
        mouse_click_listener(T owner) : owner_(owner) {}

        void do_click(Sint32 x, Sint32 y, int count, 
                      Uint8 state, Uint8 bstate, SDLMod mod) {
            owner_->click(x, y, count, state, bstate, mod);
        }
    private:
        T owner_;
    };
}

#endif
