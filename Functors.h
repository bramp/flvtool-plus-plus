#ifndef _FUNCTOR_H_
#define _FUNCTOR_H_

/// Used to delete objects
/// ie for_each(v.begin(), v.end(), DeleteObject());
struct DeleteObject {
	template<typename T>
		void operator()(const T* ptr) const {
			if (ptr) {
				delete ptr;
			}
		}
};

/// Used to delete pair.second
struct DeletePairSecond {
	template<typename T>
		void operator()(const T &p) const {
			delete p.second;
		}
};

#endif
