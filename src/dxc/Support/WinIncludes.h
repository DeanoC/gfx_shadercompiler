//===- WinIncludes.h --------------------------------------------*- C++ -*-===//
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// WinIncludes.h                                                             //
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
// This file is distributed under the University of Illinois Open Source     //
// License. See LICENSE.TXT for details.                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef _MSC_VER

#define NOATOM 1
#define NOGDICAPMASKS 1
#define NOMETAFILE 1
#define NOMINMAX 1
#define NOOPENFILE 1
#define NORASTEROPS 1
#define NOSCROLL 1
#define NOSOUND 1
#define NOSYSMETRICS 1
#define NOWH 1
#define NOCOMM 1
#define NOKANJI 1
#define NOCRYPT 1
#define NOMCX 1
#define WIN32_LEAN_AND_MEAN 1
#define VC_EXTRALEAN 1
#define NONAMELESSSTRUCT 1

#include <windows.h>
#include <unknwn.h>

//===--------------------- COM Pointer Types ------------------------------===//

class CAllocator {
public:
	static void *Reallocate(void *p, size_t nBytes) throw();
	static void *Allocate(size_t nBytes) throw();
	static void Free(void *p) throw();
};

template <class T> class CComPtrBase {
protected:
	CComPtrBase() throw() { p = nullptr; }
	CComPtrBase(T *lp) throw() {
		p = lp;
		if (p != nullptr)
			p->AddRef();
	}
	void Swap(CComPtrBase &other) {
		T *pTemp = p;
		p = other.p;
		other.p = pTemp;
	}

public:
	~CComPtrBase() throw() {
		if (p) {
			p->Release();
			p = nullptr;
		}
	}
	operator T *() const throw() { return p; }
	T &operator*() const { return *p; }
	T *operator->() const { return p; }
	T **operator&() throw() {
		assert(p == nullptr);
		return &p;
	}
	bool operator!() const throw() { return (p == nullptr); }
	bool operator<(T *pT) const throw() { return p < pT; }
	bool operator!=(T *pT) const { return !operator==(pT); }
	bool operator==(T *pT) const throw() { return p == pT; }

	// Release the interface and set to nullptr
	void Release() throw() {
		T *pTemp = p;
		if (pTemp) {
			p = nullptr;
			pTemp->Release();
		}
	}

	// Attach to an existing interface (does not AddRef)
	void Attach(T *p2) throw() {
		if (p) {
			ULONG ref = p->Release();
			(void)(ref);
			// Attaching to the same object only works if duplicate references are
			// being coalesced.  Otherwise re-attaching will cause the pointer to be
			// released and may cause a crash on a subsequent dereference.
			assert(ref != 0 || p2 != p);
		}
		p = p2;
	}

	// Detach the interface (does not Release)
	T *Detach() throw() {
		T *pt = p;
		p = nullptr;
		return pt;
	}

	HRESULT CopyTo(T **ppT) throw() {
		assert(ppT != nullptr);
		if (ppT == nullptr)
			return E_POINTER;
		*ppT = p;
		if (p)
			p->AddRef();
		return S_OK;
	}

	template <class Q> HRESULT QueryInterface(Q **pp) const throw() {
		assert(pp != nullptr);
		return p->QueryInterface(__uuidof(Q), (void **)pp);
	}

	T *p;
};

template <class T> class CComPtr : public CComPtrBase<T> {
public:
	CComPtr() throw() {}
	CComPtr(T *lp) throw() : CComPtrBase<T>(lp) {}
	CComPtr(const CComPtr<T> &lp) throw() : CComPtrBase<T>(lp.p) {}
	T *operator=(T *lp) throw() {
		if (*this != lp) {
			CComPtr(lp).Swap(*this);
		}
		return *this;
	}

	inline bool IsEqualObject(IUnknown *pOther) throw() {
		if (this->p == nullptr && pOther == nullptr)
			return true; // They are both NULL objects

		if (this->p == nullptr || pOther == nullptr)
			return false; // One is NULL the other is not

		CComPtr<IUnknown> punk1;
		CComPtr<IUnknown> punk2;
		this->p->QueryInterface(__uuidof(IUnknown), (void **)&punk1);
		pOther->QueryInterface(__uuidof(IUnknown), (void **)&punk2);
		return punk1 == punk2;
	}

	void ComPtrAssign(IUnknown **pp, IUnknown *lp, REFIID riid) {
		IUnknown *pTemp = *pp; // takes ownership
		if (lp == nullptr || FAILED(lp->QueryInterface(riid, (void **)pp)))
			*pp = nullptr;
		if (pTemp)
			pTemp->Release();
	}

	template <typename Q> T *operator=(const CComPtr<Q> &lp) throw() {
		if (!this->IsEqualObject(lp)) {
			ComPtrAssign((IUnknown **)&this->p, lp, __uuidof(T));
		}
		return *this;
	}

	T *operator=(const CComPtr<T> &lp) throw() {
		if (*this != lp) {
			CComPtr(lp).Swap(*this);
		}
		return *this;
	}

	CComPtr(CComPtr<T> &&lp) throw() : CComPtrBase<T>() { lp.Swap(*this); }

	T *operator=(CComPtr<T> &&lp) throw() {
		if (*this != lp) {
			CComPtr(static_cast<CComPtr &&>(lp)).Swap(*this);
		}
		return *this;
	}
};

#else // _MSC_VER

#include "dxc/Support/WinAdapter.h"

#ifdef __cplusplus
// Define operator overloads to enable bit operations on enum values that are
// used to define flags. Use DEFINE_ENUM_FLAG_OPERATORS(YOUR_TYPE) to enable these
// operators on YOUR_TYPE.
extern "C++" {
template <size_t S>
struct _ENUM_FLAG_INTEGER_FOR_SIZE;

template <>
struct _ENUM_FLAG_INTEGER_FOR_SIZE<1>
{
	typedef int8_t type;
};

template <>
struct _ENUM_FLAG_INTEGER_FOR_SIZE<2>
{
	typedef int16_t type;
};

template <>
struct _ENUM_FLAG_INTEGER_FOR_SIZE<4>
{
	typedef int32_t type;
};

// used as an approximation of std::underlying_type<T>
template <class T>
struct _ENUM_FLAG_SIZED_INTEGER
{
	typedef typename _ENUM_FLAG_INTEGER_FOR_SIZE<sizeof(T)>::type type;
};

}
#define DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE) \
extern "C++" { \
inline ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)a) | ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
inline ENUMTYPE &operator |= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type &)a) |= ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
inline ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)a) & ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
inline ENUMTYPE &operator &= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type &)a) &= ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
inline ENUMTYPE operator ~ (ENUMTYPE a) { return ENUMTYPE(~((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)a)); } \
inline ENUMTYPE operator ^ (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)a) ^ ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
inline ENUMTYPE &operator ^= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type &)a) ^= ((_ENUM_FLAG_SIZED_INTEGER<ENUMTYPE>::type)b)); } \
}
#else
#define DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE) // NOP, C allows these operators.
#endif

#endif // _MSC_VER
