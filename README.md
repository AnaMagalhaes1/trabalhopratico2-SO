# Trabalho Prático 2 — Gerenciador de Memória Virtual

Simulador, em C, de tradução de endereços lógicos para físicos usando **TLB (FIFO)**
e **Tabela de Páginas** com paginação por demanda e substituição de páginas via
**LRU aproximado (Aging Algorithm)**.

Disciplina: Sistemas Operacionais — PUC Minas.

## Estrutura

```text
vm_manager/
├── Makefile
├── README.md
├── include/
│   ├── config.h
│   ├── tlb.h
│   ├── page_table.h
│   ├── memory.h
│   └── statistics.h
├── src/
│   ├── main.c
│   ├── tlb.c
│   ├── page_table.c
│   ├── memory.c
│   └── statistics.c
├── data/
│   └── generate_data.py
└── report/
    └── relatorio.pdf
```

## Como executar

### 1. Gerar os arquivos de entrada

```bash
cd data
python3 generate_data.py
cd ..
```

Isso cria:

- `data/BACKING_STORE.bin` — memória de retaguarda (65.536 bytes, determinística)
- `data/addresses_random.txt` — 10.000 endereços lógicos aleatórios
- `data/addresses_location.txt` — 10.000 endereços com localidade de referência

### 2. Compilar

```bash
make
```

Compila com `gcc -Wall -Wextra -O2 -std=c11`, sem warnings.

### 3. Executar

```bash
./vm < data/addresses_random.txt
./vm < data/addresses_location.txt
```

Ou usando os atalhos do Makefile:

```bash
make run-random
make run-location
```

## Saída esperada

Para cada endereço lógico, o programa imprime:

```text
Logical address: <endereço> Physical address: <endereço físico> Value: <byte lido>
```

Ao final, são exibidas as estatísticas:

```text
Number of Translated Addresses = 10000
Page Faults = ...
Page Fault Rate = ...
TLB Hits = ...
TLB Hit Rate = ...
```

## Resultados de referência (seed = 42)

| Métrica              | addresses_random.txt | addresses_location.txt |
|-----------------------|:---------------------:|:------------------------:|
| Page Faults           | 4.958                 | 1.164                    |
| Taxa de Page Faults   | 49,58%                | 11,64%                   |
| TLB Hits              | 654                   | 7.925                    |
| Taxa de TLB Hit       | 6,54%                 | 79,25%                   |

A diferença entre os dois arquivos evidencia o impacto da localidade de
referência: endereços puramente aleatórios geram baixo reaproveitamento de
páginas e da TLB, enquanto endereços com localidade (85% dos acessos em
páginas próximas) elevam fortemente a taxa de acerto da TLB e reduzem as
faltas de página.

## Arquitetura

- **`tlb.c`** — TLB de 16 entradas, com busca linear e substituição **FIFO**
  (índice circular `fifo_next`).
- **`page_table.c`** — tabela de páginas com 256 entradas; cada entrada guarda
  `frame`, `valid`, `reference_bit` e `aging_counter` (8 bits) para o LRU
  aproximado.
- **`memory.c`** — memória física (128 quadros × 256 bytes), tratamento de
  page faults (busca de quadro livre → seleção de vítima via Aging →
  invalidação na tabela de páginas e na TLB → leitura do `BACKING_STORE.bin`
  via `fseek`/`fread`).
- **`statistics.c`** — contadores de endereços traduzidos, page faults e TLB
  hits, e cálculo das taxas finais.
- **`main.c`** — laço principal: extrai página/offset, consulta TLB →
  tabela de páginas → trata page fault se necessário, atualiza referência e
  aging, calcula o endereço físico e imprime o resultado.

## Algoritmo de substituição (Aging / LRU aproximado)

A cada acesso à memória:

1. O `aging_counter` de todas as páginas válidas é deslocado **1 bit para a direita**;
2. O `reference_bit` da página é inserido no **bit mais significativo** do contador;
3. O `reference_bit` é zerado.

Quando uma falta de página ocorre e não há quadro livre, é escolhida como
vítima a página válida com o **menor valor de `aging_counter`**.



