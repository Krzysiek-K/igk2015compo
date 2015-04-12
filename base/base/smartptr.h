
#ifndef _SMARTPTR_H
#define _SMARTPTR_H


#include <stdio.h>
#include <assert.h>


namespace base {

class Collectable;
template<class _T> class weak_ptr;
template<class _T> class shared_ptr;



template<class _T>
class traced_ptr {
public:


	traced_ptr()
	{
		next = prev = this;
		ptr = NULL;
	}
	
	traced_ptr(const traced_ptr<_T> &tp)
	{
		next = prev = this;
		BindTo(tp);
	}
	
	~traced_ptr()
	{
		Unbind();
	}

	_T *get() const { return ptr; }

	_T &operator *() const  { assert(ptr!=NULL); return *ptr; }
	_T *operator ->() const { assert(ptr!=NULL); return  ptr; }

	traced_ptr<_T> &operator =(const traced_ptr<_T> &tp)
	{
		BindTo(tp);
		return *this;
	}
	
	bool operator ==(const traced_ptr<_T> &tp) const { return (ptr==tp.ptr);	}
	bool operator ==(const _T *p) const { return (ptr==p); }

	bool operator !=(const traced_ptr<_T> &tp) const { return (ptr!=tp.ptr);	}
	bool operator !=(const _T *p) const { return (ptr!=p); }

	void set_all(_T *new_ptr)
	{
		SetRing(new_ptr);
	}
	
	void init_new(_T *new_ptr)
	{
		Unbind();
		ptr = new_ptr;
	}

	bool is_alone() const
	{
		return (next==this);
	}
	
	int ring_size() const
	{
		traced_ptr<_T> *p = (traced_ptr<_T>*)this;
		int cnt=0;
		do
		{
			cnt++;
			p = p->next;
		} while(p!=this);
		return cnt;
	}

	//operator _T*()
	//{
	//	return ptr;
	//}
private:
	template<class _T> friend class weak_ptr;
	_T				*ptr;
	traced_ptr<_T>	*next,*prev;


	void Unbind()
	{
		next->prev = prev;
		prev->next = next;
		next = prev = this;
	}
	
	void BindTo(const traced_ptr<_T> &tp)
	{
		Unbind();
		prev = (traced_ptr<_T>*)&tp;
		next = tp.next;
		prev->next = this;
		next->prev = this;
		ptr = tp.ptr;
	}
	
	void SetRing(_T *new_ptr)
	{
		traced_ptr<_T> *p = this;
		do
		{
			p->ptr = new_ptr;
			p = p->next;
		} while(p!=this);
	}

};


template<class _T>
class weak_ptr {
public:

	weak_ptr() {}
	weak_ptr(_T *p) { if(p) tracer = p->_this; }
	weak_ptr(const weak_ptr<_T> &wp) : tracer(wp.tracer) { }
	weak_ptr(const shared_ptr<_T> &sp) { if(sp!=NULL) tracer = sp->_this; }

	_T *get() const { return (_T*)(tracer.ptr); }

	_T &operator *() const  { assert(tracer.ptr!=NULL); return *(_T*)(tracer.ptr); }
	_T *operator ->() const { assert(tracer.ptr!=NULL); return  (_T*)(tracer.ptr); }


	weak_ptr<_T> &operator =(_T *p)
	{
		if(p)	tracer = p->_this;
		else	tracer = traced_ptr<Collectable>();
		return *this;
	}
	
	weak_ptr<_T> &operator =(const weak_ptr<_T> &wp)
	{
		tracer = wp.tracer;
		return *this;
	}
	
	weak_ptr<_T> &operator =(const shared_ptr<_T> &sp)
	{
		return operator =(sp.get());
	}
	
	bool operator ==(const weak_ptr<_T> &wp) const { return (tracer==wp.tracer);	}
	bool operator ==(const shared_ptr<_T> &sp) const { return (tracer==sp.get());	}
	bool operator ==(const _T *p) const { return (tracer==p); }

	bool operator !=(const weak_ptr<_T> &wp) const { return (tracer!=wp.tracer);	}
	bool operator !=(const shared_ptr<_T> &sp) const { return (tracer!=sp.get());	}
	bool operator !=(const _T *p) const { return (tracer!=p); }

	//operator _T*()
	//{
	//	return ptr;
	//}
private:
	template<class _T> friend class shared_ptr;
	traced_ptr<Collectable> tracer;

};

template<class _T>
class shared_ptr {
public:

	shared_ptr() : ptr(NULL) {}
	shared_ptr(_T *p) : ptr(NULL) { SetPointer(p); }
	shared_ptr(const shared_ptr<_T> &sp) : ptr(NULL) { SetPointer(sp.ptr); }
	shared_ptr(const weak_ptr<_T> &wp) : ptr(NULL) { SetPointer(wp.get()); }

	virtual ~shared_ptr() {SetPointer(NULL);} 


	_T *get() const { return ptr; }

	_T &operator *() const  { assert(ptr!=NULL); return *ptr; }
	_T *operator ->() const { assert(ptr!=NULL); return  ptr; }


	shared_ptr<_T> &operator =(_T *p)
	{
		SetPointer(p);
		return *this;
	}
	
	shared_ptr<_T> &operator =(const shared_ptr<_T> &sp)
	{
		SetPointer(sp.ptr);
		return *this;
	}
	
	shared_ptr<_T> &operator =(const weak_ptr<_T> &wp)
	{
		SetPointer(wp.get());
		return *this;
	}

	//operator _T*()
	//{
	//	return ptr;
	//}
	
	bool operator ==(const shared_ptr<_T> &sp) const { return (ptr==sp.ptr);	}
	bool operator ==(const weak_ptr<_T> &wp) const { return (ptr==wp.get());	}
	bool operator ==(const _T *p) const { return (ptr==p); }

	bool operator !=(const shared_ptr<_T> &sp) const { return (ptr!=sp.ptr);	}
	bool operator !=(const weak_ptr<_T> &wp) const { return (ptr!=wp.get());	}
	bool operator !=(const _T *p) const { return (ptr!=p); }


private:
	template<class _T> friend class weak_ptr;
	_T	*ptr;


	void SetPointer(_T *p)
	{
		if(p  ) p->_add_ref();
		if(ptr) ptr->_release();
		ptr = p;
	}
};


class Collectable {
private:
	template<class _T> friend class weak_ptr;
	template<class _T> friend class shared_ptr;
	int							_ref_count;
	traced_ptr<Collectable>		_this;

public:
	Collectable() : _ref_count(0)
	{
		_this.init_new(this);
	}
	
	virtual ~Collectable()
	{
		_this.set_all(NULL);
	}


	void _add_ref()
	{
		_ref_count++;
		assert(_ref_count>0);
	}

	void _release()
	{
		assert(_ref_count>0);
		_ref_count--;
		if(_ref_count<=0)
			delete this;
	}
};


}


#endif
