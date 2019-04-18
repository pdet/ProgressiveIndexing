//
// Created by Pedro Holanda on 17/04/19.
//

#ifndef PROGRESSIVEINDEXING_REWIRING_H
#define PROGRESSIVEINDEXING_REWIRING_H

#include "assert.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <err.h>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "abrt.h"

#define SYS_memfd_create 319
namespace rewiring
{
    constexpr const char *HUGETLBFS_MOUNTPOINT = "/export/scratch1/home/holanda/mnt/hugetlbfs/";

    template<std::size_t PAGE_SIZE>
    inline std::size_t ceil_to_pagesize(std::size_t size)
    {
        constexpr std::size_t PAGEMASK = PAGE_SIZE - 1;
        if (size & PAGEMASK)
            size = (size | PAGEMASK) + 1;
        assert((size & PAGEMASK) == 0);
        return size;
    }

    template<std::size_t PAGE_SIZE>
    inline std::size_t floor_to_pagesize(std::size_t size)
    {
        constexpr std::size_t PAGEMASK = ~(PAGE_SIZE - 1);
        return size & PAGEMASK;
    }


    template<bool UseHugepages>
    struct memory_file;

    template<bool UseHugepages, std::size_t CHUNK_SIZE>
    struct memory_mapping
    {
        static constexpr auto PAGE_SIZE = memory_file<UseHugepages>::PAGE_SIZE;
        static constexpr auto PAGES_PER_CHUNK = CHUNK_SIZE / PAGE_SIZE;
        static_assert(PAGES_PER_CHUNK != 0, "chunk too small");
        static_assert(PAGES_PER_CHUNK * PAGE_SIZE == CHUNK_SIZE, "chunk not a whole multiple of a page");

        friend void swap(memory_mapping &first, memory_mapping &second);

        memory_mapping() : addr(nullptr), chunks(nullptr) { };

        long operator[](std::size_t n) const;
        void unmap();

        void *addr;
        long *chunks; // stores the offsets of the chunks
        std::size_t size;
        int fd;
    };

    template<bool UseHugepages>
    struct memory_file
    {
        static constexpr bool HUGEPAGES = UseHugepages;
        static constexpr std::size_t PAGE_SIZE = HUGEPAGES ? (2 * 1024 * 1024) : (4 * 1024);

        memory_file(const char *filename);
        memory_file(const memory_file&) = delete;
        ~memory_file();

        void resize(std::size_t size);

        template<std::size_t CHUNK_SIZE = PAGE_SIZE>
        memory_mapping<UseHugepages, CHUNK_SIZE> map(std::size_t size, long offset);

        std::string path;
        std::size_t size = 0;
        int fd;
    };


/*-- Memory file -----------------------------------------------------------------------------------------------------*/
    template<>
     memory_file<false>::memory_file(const char *filename)
    {
        path = filename;
        fd = syscall(SYS_memfd_create, filename, 0);
    }

    template<>
     memory_file<true>::memory_file(const char *filename)
    {
        path = std::string{HUGETLBFS_MOUNTPOINT}.append(filename);
        fd = open(path.c_str(), O_RDWR | O_CREAT, 0755);
    }

    template<bool UseHugepages>
     memory_file<UseHugepages>::~memory_file()
    {
        if (close(fd))
            warn("closing file \"%s\" failed", path.c_str());

        if  (UseHugepages) {
            if (unlink(path.c_str()))
                warn("unlinking file \"%s\" failed", path.c_str());
        }
    }

    template<bool UseHugepages>
    void memory_file<UseHugepages>::resize(std::size_t size)
    {
        size = ceil_to_pagesize<PAGE_SIZE>(size);
        if (ftruncate(fd, size))
            abrt("ftruncate() failed");
        this->size = size;
    }

    template<bool UseHugepages>
    template<std::size_t CHUNK_SIZE>
    memory_mapping<UseHugepages, CHUNK_SIZE> memory_file<UseHugepages>::map(std::size_t size, long offset)
    {
        memory_mapping<UseHugepages, CHUNK_SIZE> vm;
        vm.fd = this->fd;
        vm.size = size = ceil_to_pagesize<PAGE_SIZE>(size);
        const std::size_t num_chunks = size / CHUNK_SIZE;
        vm.chunks = new long[num_chunks];

        if (offset + size > this->size) {
            if (ftruncate(fd, offset + size))
                abrt("ftruncate() failed");
            else
                this->size = offset + size;
        }

        vm.addr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
        if (vm.addr == MAP_FAILED)
            abrt("mmap() failed");

        for (std::size_t i = 0; i != num_chunks; ++i)
            vm.chunks[i] = offset + i * CHUNK_SIZE;

        return vm;
    }


/*-- Memory mapping --------------------------------------------------------------------------------------------------*/
    template<bool UseHugepages, std::size_t CHUNK_SIZE>
    long memory_mapping<UseHugepages, CHUNK_SIZE>::operator[](std::size_t n) const
    {
        assert(n < size / CHUNK_SIZE);
        return chunks[n];
    }

    template<bool UseHugepages, std::size_t CHUNK_SIZE>
    void memory_mapping<UseHugepages, CHUNK_SIZE>::unmap()
    {
        if (munmap(addr, size))
            warn("munmap(%p) failed", addr);
        addr = nullptr;
        delete[] chunks;
        chunks = nullptr;
    }

    template<bool UseHugepages, std::size_t CHUNK_SIZE>
    inline void swap(memory_mapping<UseHugepages, CHUNK_SIZE> &vm_first, std::size_t chunkID_first,
                     memory_mapping<UseHugepages, CHUNK_SIZE> &vm_second, std::size_t chunkID_second)
    {
        assert(vm_first.fd == vm_second.fd && "different memory files");

        const int fd = vm_first.fd;
        void *addr;

        void * const addr_first = ((uint8_t*) vm_first.addr) + chunkID_first * CHUNK_SIZE;
        void * const addr_second = ((uint8_t*) vm_second.addr) + chunkID_second * CHUNK_SIZE;
        const long offset_first = vm_first[chunkID_first];
        const long offset_second = vm_second[chunkID_second];

        /* Map the address space of `vm_first' to the physical area of `vm_second'. */
        addr = mmap(addr_first, CHUNK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, offset_second);
        if (addr == MAP_FAILED)
            abrt("mmap() failed");
        assert(addr == addr_first);

        /* Map the address space of `vm_second' to the physical area of `vm_first'. */
        addr = mmap(addr_second, CHUNK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, offset_first);
        if (addr == MAP_FAILED)
            abrt("mmap() failed");
        assert(addr == addr_second);

        using std::swap;
        swap(vm_first.chunks[chunkID_first], vm_second.chunks[chunkID_second]);
    }


}

#endif //PROGRESSIVEINDEXING_REWIRING_H
