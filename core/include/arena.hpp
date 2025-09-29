#pragma once


/*
Copyright 2025 Tomas Prerovsky (cepsdev@hotmail.com).

Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to iswp128b8n writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <stddef.h>
#include <cstdlib>
#include <cstring>

template<int arena_count>
class Arena{
    struct arena_header{
     arena_header* next{};
     arena_header* prev{};
     char* available{};
     char* limit{};
     size_t counter{};
    };

    arena_header* arenahds{};
    arena_header** arenatails{};
    arena_header* freeblocks{};
    double resize_factor = 1.1;
    public:
    Arena() {
     arenahds = (arena_header*)memset(malloc(arena_count * sizeof(arena_header)),0,arena_count * sizeof(arena_header));
     arenatails = (arena_header**)memset(malloc(arena_count * sizeof(arena_header*)),0,arena_count * sizeof(arena_header*));
     freeblocks = nullptr;
     for (size_t i{}; i < arena_count; ++i){
        arenatails[i] = &arenahds[i]; 
     }
    }
    char* allocate(size_t n, size_t arena){
        if (arena >= arena_count) return nullptr;
        auto ap = arenatails[arena];
        while(ap->available + n > ap->limit) {
                        
            if(!ap->next){
                if (freeblocks) { 
                    ap->next = freeblocks;
                    freeblocks->prev = ap; 
                    freeblocks = freeblocks->next;
                    if (freeblocks) freeblocks->prev = nullptr;
                    ap = arenatails[arena] = ap->next;
                    ap->next = nullptr;
                    ap->available = (char*)ap + sizeof(arena_header);
                    ap->counter = 0;
                    continue; 
                } else {
                 size_t m = n * resize_factor + sizeof(arena_header);
                 auto ap_prev = ap;
                 ap = ap->next = (arena_header*) malloc(m);
                 if (!ap) return nullptr;
                 ap->prev = ap_prev;
                 ap->next = nullptr;
                 ap->counter = 0;
                 ap->limit = (char*)ap + m;
                 ap->available = (char*)ap + sizeof(arena_header);
                 arenatails[arena] = ap;
                 continue;
               }
            }

        }
        if (ap){
            ap->available += n;
            ++ap->counter;
            return ap->available - n;
        }
        return nullptr;
    }
    arena_header* find_page(char* mem, size_t arena){
        for(auto p = &arenahds[arena]; p; p = p->next){
            if (p->limit > mem && (char*)p < mem) return p;
        }
        return nullptr;
    }

    void free(arena_header& page, size_t arena){
        if (arenatails[arena] == &page) arenatails[arena] = page.prev;
        if (page.next) page.next->prev = page.prev;
        if (page.prev) page.prev->next = page.next; 
        page.next = freeblocks;
        page.prev = nullptr;
        if(freeblocks) freeblocks->prev = &page;
        freeblocks = &page;
        
    }

    char* reallocate(char* mem, size_t n_old, size_t n_new, size_t arena){
        auto new_mem = allocate(n_new, arena); //(*)
        if (!new_mem) return nullptr;
        memcpy(new_mem,mem, n_old);   
        auto page = find_page(mem,arena);
        if (page && 0 == --page->counter){
            //INVARIANT: page is not referenced && last allocation (*) hit another page
            //=> page is neither head nor tail of page list
            free(*page, arena);            
        }
        return new_mem;
    }
    void free(size_t arena){
        if (!arenahds[arena].next) return; // nothing to free
        //INVARIANT: &arenahds[arena] != arenatails[arena]
        auto t = freeblocks;
        freeblocks = arenahds[arena].next; 
        if(arenahds[arena].next) arenahds[arena].next->prev = nullptr;
        arenatails[arena]->next = t;
        if (t) t->prev = arenatails[arena];
        arenatails[arena] = &arenahds[arena];
        arenahds[arena].next = arenahds[arena].prev = nullptr;
    }
    void free(size_t arena, char* mem){
        auto page = find_page(mem,arena);
        if (page && page != &arenahds[arena] && 0 == --page->counter) free(*page, arena);    
    }
};