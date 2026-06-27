#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "config.h"
#include "page_table.h"
#include "tlb.h"

static signed char physical_memory[NUM_FRAMES][FRAME_SIZE];

/*
 * Indica qual página está carregada em cada quadro.
 * Valor -1 indica quadro livre.
 */
static int frame_to_page[NUM_FRAMES];

static FILE *backing = NULL;

void memory_init(FILE *backing_store)
{
    backing = backing_store;

    for (int i = 0; i < NUM_FRAMES; i++) {
        frame_to_page[i] = -1;

        for (int j = 0; j < FRAME_SIZE; j++) {
            physical_memory[i][j] = 0;
        }
    }
}

static int find_free_frame(void)
{
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (frame_to_page[i] == -1) {
            return i;
        }
    }

    return -1;
}

int handle_page_fault(int page)
{
    int frame = find_free_frame();

    if (frame == -1) {
        /* Não há quadro livre: seleciona página vítima via LRU aproximado (aging). */
        int victim_page = select_victim_page();
        int victim_frame = page_table_get_frame(victim_page);

        /* Invalida a entrada da página vítima na tabela de páginas. */
        page_table_invalidate(victim_page);

        /* Remove a entrada correspondente do TLB, caso exista. */
        tlb_remove(victim_page);

        frame = victim_frame;
        frame_to_page[frame] = -1;
    }

    if (backing == NULL) {
        fprintf(stderr, "Erro interno: BACKING_STORE nao inicializado.\n");
        exit(1);
    }

    /* Lê a página correspondente do BACKING_STORE.bin. */
    if (fseek(backing, page * PAGE_SIZE, SEEK_SET) != 0) {
        fprintf(stderr, "Erro: falha ao buscar pagina %d no BACKING_STORE.\n", page);
        exit(1);
    }

    size_t read_bytes = fread(physical_memory[frame], sizeof(signed char), PAGE_SIZE, backing);

    if (read_bytes != PAGE_SIZE) {
        fprintf(stderr, "Erro: falha ao ler pagina %d do BACKING_STORE.\n", page);
        exit(1);
    }

    /* Atualiza o mapeamento quadro -> página e a tabela de páginas. */
    frame_to_page[frame] = page;
    page_table_update(page, frame);

    return frame;
}

int select_victim_page(void)
{
    /*
     * Seleciona a página válida com o menor valor de aging_counter,
     * aproximando o comportamento do algoritmo LRU.
     */
    int victim = -1;
    int min_counter = 256; /* maior que qualquer valor possível (0-255) */

    for (int page = 0; page < PAGE_TABLE_SIZE; page++) {
        if (!page_table_is_valid(page)) {
            continue;
        }

        unsigned char counter = page_table_get_aging_counter(page);

        if (victim == -1 || counter < min_counter) {
            min_counter = counter;
            victim = page;
        }
    }

    return victim;
}

signed char read_memory(int frame, int offset)
{
    return physical_memory[frame][offset];
}

int get_page_loaded_in_frame(int frame)
{
    if (frame < 0 || frame >= NUM_FRAMES) {
        return -1;
    }

    return frame_to_page[frame];
}
