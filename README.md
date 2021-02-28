Trimadap is a small tool to trim adapter sequences from Illumina data. It
performs SSE2-SW between each read and each adapter sequence and identifies
adapter sequences with a few heuristic rules which can be found in the
`ta_trim1()` function in `trimadap.c`. The default adapters it uses are
included in `illumina.txt`. These are typical Illumina adapters from
paired-end sequencing.

Trimadap is designed as an on-the-fly stream filter. It is very fast. In the
multi-threading mode, it is as fast as reading through a gzip-compressed FASTQ
file. On the other hand, trimadap is very conservative. It is not good in terms
of accuracy as of now. I will probably fine tune the heuristic rules in
future. This should not be hard in principle, but it takes development time.

## Parameters

   Parameter  | Type | Description | Default/Note
--------------|------|-------------|-------------
  `-3` | STR    | 3\'-end adapter | DNBSEQ Forward filter
  `-5` | STR    | 5\'-end adapter | DNBSEQ Reverse filter
  `-l` | INT    | min length     | 8
  `-s` | INT    | min score      | 15
  `-t` | INT    | trim down masked part (Xs) | don't trim
  `-d` | FLOAT  | max difference | 0.150
  `-r` | INT    | min read length (w/ trimmed bases counted out) to output | 35
  `-p` | INT    | number of trimmer threads | 1
  `-m` | CHAR   | masker character (X or N) | X
  `-q` |        | perform basic fq qc | &nbsp;
  `-h` |        | print help message | &nbsp;
  `-v` |        | print version number |  &nbsp;

## Examples

* process DNBSEQ SE reads data
```
trimadap \
  -3 AAGTCGGAGGCCAAGCGGTCTTAGGAAGACAA \
  -l 5 -t 50 -r 35 a.fq > a_masked.fq
```

* process DNBSEQ PE reads data and mask adaptor sequences as Ns
```
trimadap \
  -3 AAGTCGGAGGCCAAGCGGTCTTAGGAAGACAA \
  -5 AAGTCGGATCGTAGCCATGTCGTTCTGTGAGCCAAGGAGTTG \
  -l 5 -t 50 -r 35 -m N a.fq > a_masked.fq
```
