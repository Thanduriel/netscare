#pragma once

#include <Windows.h>
#include <cassert>

using Vtable = DWORD;

template<typename FnT>
class VTableHook
{
public:
	template<typename T>
	VTableHook(T* _object)
		: m_baseObject(reinterpret_cast<DWORD*>(_object)),
		m_oldVtable(GetVtable(_object)),
		m_entryCount(GetVtableCount(m_oldVtable))
	{
		DWORD dwOld;
		VirtualProtect(m_oldVtable, m_entryCount, PAGE_EXECUTE_READWRITE, &dwOld);

		m_newVtable = new DWORD[m_entryCount];
		memcpy(m_newVtable, m_oldVtable, sizeof(DWORD) * m_entryCount);
		*m_baseObject = 0;// reinterpret_cast<DWORD>(m_newVtable);
	}

	~VTableHook()
	{
		delete m_newVtable;
	}

	void AddHook(FnT _function, unsigned _index)
	{
		assert(_index < m_entryCount);

		m_newVtable[_index] = *reinterpret_cast<DWORD*>(_function);
	}

	// points to the first element of the objects vtable
	template<typename T>
	static DWORD* GetVtable(T* _object)
	{
		return reinterpret_cast<DWORD*>(*reinterpret_cast<DWORD*>(_object));
	}

private:
	unsigned GetVtableCount(Vtable* _vtable)
	{
		DWORD dwIndex = 0;

		for (dwIndex = 0; _vtable[dwIndex]; dwIndex++)
		{
			if (IsBadCodePtr((FARPROC)_vtable[dwIndex]))
			{
				break;
			}
		}
		return dwIndex;
	}

	DWORD* m_baseObject;
	Vtable* m_newVtable;
	Vtable* m_oldVtable;
	Vtable* m_vtable;
	unsigned m_entryCount;
};