#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
// #include <pgm.h>

uint8_t* readPGM(int *lin, int *col, char *image_path) {
  FILE *f_read;
  char type[2];
  int maxVal;

  f_read = fopen(image_path, "r");
  if(!f_read) {
    perror("Erro ao abrir imagem");
    exit(1);
  }

  fscanf(f_read, "%s", type);
  if (strcmp(type,"P2")!=0) {
    perror("Formato nao eh P2");
    if(fclose(f_read)) {
      perror("Erro ao fechar arquivo");
      exit(1);
    }
  }

  fscanf(f_read, "%d %d", col, lin);
  int numCol = *col;
  int numLin = *lin;
  // printf("\n\nlin - %d\t col - %d *** %zu\n\n", numCol, numLin, numCol * numLin * sizeof(uint8_t));
  fscanf(f_read, "%d", &maxVal);
  uint8_t *img = malloc(numCol * numLin * sizeof(uint8_t));

  for (int i = 0; i < numLin; i++) {
    // printf("\n\ti:%d\n", i);
    for (int j = 0; j < numCol; j++) {
      int u;
      fscanf(f_read, "%d", &u);
      // if(strcmp(u, "\n") == 0) continue;
      img[i * numCol + j] = (uint8_t)u;
      // printf(" %d", j);
    }
  }

  if(fclose(f_read)) {
    perror("Erro ao fechar arquivo");
    exit(1);
  }

  return img;
}

void printPGM(int lin, int col, uint8_t *img, char *image_path) {
  FILE *f_write;

  f_write = fopen(image_path, "w");
  // printf("Sizeof printPGM: %zu\n", sizeof(uint8_t) * col * lin);

  if(!f_write) {
    perror("Erro ao abrir imagem");
    exit(1);
  }

  fprintf(f_write, "P2\n");

  fprintf(f_write, "%d %d\n", col, lin);

  fprintf(f_write, "255\n");

  for (int i = 0; i < lin; ++i) {
    for (int j = 0; j < col - 1; ++j) {
      fprintf(f_write, "%d  ", img[i * col + j]);
    }
    fprintf(f_write, "%d\n", img[i * col + (col - 1)]);
  }

  if(fclose(f_write)) {
    perror("Erro ao fechar arquivo");
    exit(1);
  }
}

uint8_t* getR(int lin, int col, uint8_t *img, int k) {
  uint8_t *R = malloc(sizeof(uint8_t) * col * lin);

  for (int i = 0; i < lin; ++i) {
    for (int j = 0; j < col; ++j) {
      R[i * col + j] = (img[i * col + j]) & ((1 << k) - 1);
      img[i * col + j] = (uint8_t)(img[i * col + j] - R[i * col + j]);
      // printf("\t%d\n", R[i * col + j]);
    }
  }

  return R;
}

int encryptPos(int k1, int x, int s) {
  return (k1 + 17 *  x) % s;
}

int gcdOne(int x) {
  for(int k = x - 1; k > 0; k -= 2) {
    int gcd = 1;
    for(int i = 2; i <= x && i <= k; ++i) {
      if(x % i == 0 && k % i == 0 && i > 1) {
        gcd = i;
        break;
      }
    }
    if(gcd == 1) {
      return k;
    }
  }

  return 1;
}

uint8_t *getE2(int lin, int col, uint8_t *img, int k) {
  uint8_t prop = 8 / k;
  int size = lin * col * prop;
  int k1 = gcdOne(size);
  uint8_t *E2 = malloc(sizeof(uint8_t) * size);

  printf("K1: %d, s: %d\n", k1, size);

  for (int i = 0; i < lin; ++i) {
    for (int j = 0; j < col; ++j) {
      uint8_t e = img[i * col + j];
      for (int l = 0; l < prop ; ++l) {
        // printf("\t%d %d %d -- ", i, j, l);
        // printf("\t%zu -", (e << k * l) & 0xFF);
        E2[encryptPos(k1, ((i * col) + j + l), size)] = (uint8_t)((e << k * l) & 0xFF) >> (8 - k);
        // E2[(i * col) + j + l] = (uint8_t)(((e << k * l) & 0xFF) >> (8 - k));
        // printf("%d\n", E2[(i * col) + (j * prop) + l]);
        // printf(" %zu\n", E2[j * lin + i + l]);
      }
      // printf("\t%d\n", e);
    }
  }

  return E2;
}

uint8_t* decrypt(uint8_t* hideout, int hideoutSize, int refugeeSize, int k, int lin2, int col2) {
  int eSize = refugeeSize * (8 / k);
  int prop = (8 / k);
  int k1 = gcdOne(eSize);
  uint8_t *extracted = malloc(sizeof(uint8_t) * eSize);
  uint8_t *f = malloc(sizeof(uint8_t) * refugeeSize);

  for(int i = 0; i < eSize; ++i) { // TESTED -- OK
    extracted[i] = (uint8_t)(((hideout[encryptPos(k1, i, eSize)] << (8 - k))  & 0xFF) >> (8 - k));
  }
  printPGM(lin2, col2, extracted, "./extracted.ascii.pgm"); // E == extracted
  // OK so far

  // 0 -> 4 -> 8

  for(int i = 0; i < refugeeSize; ++i) {
    uint8_t sum = 0;
    for(int j = 0; j < prop; ++j) {
      uint8_t elem = extracted[i * prop + j];
      unsigned int shift = (8 - k) - j * k;
      // elem << (6 - j * 2)
      sum = (uint8_t)(sum + (uint8_t)((elem << shift) & 0xFF));
      // printf("i = %d, shift = %d, val: %u ", j, shift, elem);
      // printf(" -- sum = %d, extracted = %u\n", sum, ((elem << shift) & 0xFF));
    // printf("%d -- %d\n", i, i * prop + j);
    }
    // break;
    f[i] = sum;
  }
  free(extracted);

  return f;
}

void hideImage(uint8_t* hideout, uint8_t* refugee, int size) { // OK
  for(int i = 0; i < size; ++i) {
    // printf("%u & %u == %u\n", hideout[i], refugee[i], (hideout[i] | refugee[i]));
    hideout[i] = (uint8_t)(hideout[i] | refugee[i]);
  }
}


int main(int argc, char *argv[]) {
  int lin, col;
  int lin2, col2;
  uint8_t* hideout = readPGM(&lin, &col, argv[1]);
  uint8_t* refugee = readPGM(&lin2, &col2, argv[2]);
  int k = 2;

  if((col2 * 8 / k) > col || (lin2 > lin)) {
    printf("Tamanho da imagem 1 nao eh grande o suficiente para esconder a 2\n");
    return 1;
  }

  uint8_t *R = getR(lin, col, hideout, k);
  // for (uint8_t i = 0; i < lin2; ++i) {
  //  for (uint8_t j = 0; j < col2; ++j) {
  //    printf("%d ", R[i * lin2 + j]);
  //  }
  //  printf("\n");
  // }

  uint8_t *E2 = getE2(lin2, col2, refugee, k);
  // printPGM(lin2, col2, E2, "./E.ascii.pgm");

  hideImage(hideout, E2, lin2 * col2 * (8/k));
  printPGM(lin, col, hideout, "./hideout-encr.ascii.pgm");

  uint8_t* found = decrypt(hideout, lin * col, lin2 * col2, k, lin2, col2);
  printPGM(lin2, col2, refugee, "./refugee-encr.ascii.pgm");
  // printPGM(lin2, col2 * 8 / k, found, "./extracted.ascii.pgm");

  free(R);free(E2);free(found);
  return 0;
}
