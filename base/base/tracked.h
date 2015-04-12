
#ifndef _TRACKED_H
#define _TRACKED_H



namespace base {



// -------------------------------- Tracked --------------------------------

template<class T>
class Tracked {
public:

    class iterator {
    public:

        iterator() : _ptr(0) {}

        T &operator *()  const { return *_ptr; }
        T *operator ->() const { return _ptr; }

        iterator &operator ++() { _ptr = _ptr->_next; return *this; }
        iterator &operator --() { _ptr = (_ptr ? _ptr->_prev : T::Tail()); return *this; }

        iterator operator ++(int) { iterator pp(_ptr); _ptr = _ptr->_next; return pp; }
        iterator operator --(int) { iterator pp(_ptr); _ptr = (_ptr ? _ptr->_prev : T::Tail()); return pp; }

        bool operator ==(const iterator &i) const { return _ptr == i._ptr; }
        bool operator !=(const iterator &i) const { return _ptr != i._ptr; }

    private:
        friend class Tracked;

        T *_ptr;

        iterator(T *p) : _ptr(p) {}
    };

    static iterator begin() { return iterator(_head()); }
    static iterator end()   { return iterator(); }


protected:
    Tracked()                                       { _register(); }
    Tracked(const Tracked<T> &t)                   { _register(); }
    Tracked<T> &operator =(const Tracked<T> &t)   {}

    ~Tracked() { _unregister(); }

private:
    friend class Tracked::iterator;

    T  *_prev;
    T  *_next;

    void _register()
    {
        _prev = Tail();
        _next = 0;
        Tail() = (T*)this;
        if(_prev)   _prev->_next = (T*)this;
        else        _head() = (T*)this;
    }

    void _unregister()
    {
        if(_prev) _prev->_next = _next;
        else      _head() = _next;
        if(_next) _next->_prev = _prev;
        else      Tail() = _prev;
    }

    static T *&_head()
    {
        static T *head = 0;
        return head;
    }

    static T *&Tail()
    {
        static T *tail = 0;
        return tail;
    }
};


}



#endif
