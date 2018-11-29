#pragma once
#include <memory>


//�萔�o�b�t�@�\�z�p
//DirectXMath�̃x�N�g���ƍs���16byte���E�ɔz�u����K�v������
template <typename T, size_t ALIGN = __alignof(T)>
class AlignedObject
{
public:
	//_aligned_free Helper ��O�����p
	struct AlignedMallocHelper
	{
		AlignedMallocHelper(void* p = nullptr) : ptr(p) {}
		~AlignedMallocHelper() { if (ptr)_aligned_free(ptr); }
		void disable() { ptr = nullptr; }
		void* ptr;
	};

	AlignedObject() {
		AlignedMallocHelper p(_aligned_malloc(sizeof(T), ALIGN));
		pObj = new(p.ptr) T();
		// �R���X�g���N�^�ŗ�O������&catch��p��free
		p.disable();
	}
	~AlignedObject() {
		if (pObj) {
			pObj->~T();
			_aligned_free(pObj);
		}
	}

	T* get() { return pObj; }
	const T* get() const { return pObj; }
	T* operator->() { return pObj; }
	const T* operator->() const { return pObj; }
	T& operator*() { return *pObj; }
	const T& operator*() const { return *pObj; }

	explicit operator bool() const { return pObj != nullptr; }

	//�R�s�[�s��
	AlignedObject(const AlignedObject& o) = delete;
	const AlignedObject& operator=(const AlignedObject& o) = delete;
private:
	T * pObj;
};


// �t�@�C���ǂݍ���
struct FileRead
{
	FileRead() = delete;
	FileRead(const wchar_t* fpath);

	const void* get() const { return Bin.get(); }
	int size() const { return nSize; }
private:
	int nSize = 0;
	std::unique_ptr<char> Bin;
};