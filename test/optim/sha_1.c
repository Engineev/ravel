// Compute and Crack SHA-1
// by zzk

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int hex2int(char *x) {
  int i;
  int result = 0;
  int len = strlen(x);
  for (i = 0; i < len; i++) {
    int digit = x[i];
    if (digit >= 48 && digit <= 57)
      result = result * 16 + digit - 48;
    else if (digit >= 65 && digit <= 70)
      result = result * 16 + digit - 65 + 10;
    else if (digit >= 97 && digit <= 102)
      result = result * 16 + digit - 97 + 10;
    else
      return 0;
  }
  return result;
}

char asciiTable[] = " !\"#$%&'()*+,-./"
                    "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
                    "abcdefghijklmnopqrstuvwxyz{|}~";
char int2chr(int x) {
  if (x >= 32 && x <= 126)
    return asciiTable[x - 32];
  return 0;
}
char *toStringHex(int x) {
  char *ret = malloc(9);
  ret[8] = '\0';
  int i;
  int pos = 0;
  for (i = 28; i >= 0; i = i - 4) {
    int digit = (x >> i) & 15;
    if (digit < 10)
      ret[pos++] = int2chr(48 + digit);
    else
      ret[pos++] = int2chr(65 + digit - 10);
  }
  return ret;
}
int rotate_left(int x, int shift) {
  if (shift == 1)
    return ((x & 2147483647) << 1) | ((x >> 31) & 1);
  if (shift == 31)
    return ((x & 1) << 31) | ((x >> 1) & 2147483647);
  return ((x & ((1 << (32 - shift)) - 1)) << shift) |
      ((x >> (32 - shift)) & ((1 << shift) - 1));
}
// to avoid possible undefined behaviour when overflow
int add(int x, int y) {
  int low = (x & 65535) + (y & 65535);
  int high = (((x >> 16) & 65535) + ((y >> 16) & 65535) + (low >> 16)) & 65535;
  return (high << 16) | (low & 65535);
}
int lohi(int lo, int hi) { return lo | (hi << 16); }

int MAXCHUNK = 100;
int MAXLENGTH;
int **chunks;
int *inputBuffer;
int outputBuffer[5];

void initializeGlobalVariables() {
  MAXLENGTH = (MAXCHUNK - 1) * 64 - 16;
  chunks = malloc(sizeof(int *) * MAXCHUNK);
  for (int i = 0; i < MAXCHUNK; ++i)
    chunks[i] = malloc(sizeof(int) * 80);
  inputBuffer = malloc(sizeof(int) * MAXLENGTH);
}

int *sha1(int *input, int length) {
  int nChunk = (length + 64 - 56) / 64 + 1;
  if (nChunk > MAXCHUNK) {
    puts("nChunk > MAXCHUNK!");
    return NULL;
  }
  int i;
  int j;
  for (i = 0; i < nChunk; i++)
    for (j = 0; j < 80; j++)
      chunks[i][j] = 0;
  for (i = 0; i < length; i++)
    chunks[i / 64][i % 64 / 4] =
        chunks[i / 64][i % 64 / 4] | (input[i] << ((3 - i % 4) * 8));
  chunks[i / 64][i % 64 / 4] =
      chunks[i / 64][i % 64 / 4] | (128 << ((3 - i % 4) * 8));
  chunks[nChunk - 1][15] = length << 3;
  chunks[nChunk - 1][14] = (length >> 29) & 7;

  int h0 = 1732584193;         // 0x67452301
  int h1 = lohi(43913, 61389); // 0xEFCDAB89
  int h2 = lohi(56574, 39098); // 0x98BADCFE
  int h3 = 271733878;          // 0x10325476
  int h4 = lohi(57840, 50130); // 0xC3D2E1F0
  for (i = 0; i < nChunk; i++) {
    for (j = 16; j < 80; j++)
      chunks[i][j] = rotate_left(chunks[i][j - 3] ^ chunks[i][j - 8] ^
                                     chunks[i][j - 14] ^ chunks[i][j - 16],
                                 1);

    int a = h0;
    int b = h1;
    int c = h2;
    int d = h3;
    int e = h4;
    for (j = 0; j < 80; j++) {
      int f;
      int k;
      if (j < 20) {
        f = (b & c) | ((~b) & d);
        k = 1518500249; // 0x5A827999
      } else if (j < 40) {
        f = b ^ c ^ d;
        k = 1859775393; // 0x6ED9EBA1
      } else if (j < 60) {
        f = (b & c) | (b & d) | (c & d);
        k = lohi(48348, 36635); // 0x8F1BBCDC
      } else {
        f = b ^ c ^ d;
        k = lohi(49622, 51810); // 0xCA62C1D6
      }
      int temp = add(add(add(rotate_left(a, 5), e), add(f, k)), chunks[i][j]);
      e = d;
      d = c;
      c = rotate_left(b, 30);
      b = a;
      a = temp;
    }
    h0 = add(h0, a);
    h1 = add(h1, b);
    h2 = add(h2, c);
    h3 = add(h3, d);
    h4 = add(h4, e);
  }
  outputBuffer[0] = h0;
  outputBuffer[1] = h1;
  outputBuffer[2] = h2;
  outputBuffer[3] = h3;
  outputBuffer[4] = h4;
  return outputBuffer;
}

void computeSHA1(char *input) {
  int i;
  int len = strlen(input);
  for (i = 0; i < len; i++)
    inputBuffer[i] = input[i];
  int *result = sha1(inputBuffer, len);
  for (i = 0; i < 5; i++)
    printf("%s", toStringHex(result[i]));
  puts("");
}

int nextLetter(int now) {
  if (now == 122) //'z'
    return -1;
  if (now == 90) //'Z'
    return 97;   //'a'
  if (now == 57) //'9'
    return 65;
  return now + 1;
}

int nextText(int *now, int length) {
  int i;
  for (i = length - 1; i >= 0; i--) {
    now[i] = nextLetter(now[i]);
    if (now[i] == -1)
      now[i] = 48; //'0'
    else
      return 1;
  }
  return 0;
}

int array_equal(int *a, int *b, int len) {
  int i;
  for (i = 0; i < len; i++)
    if (a[i] != b[i])
      return 0;
  return 1;
}

void crackSHA1(char *input) {
  int *target = malloc(sizeof(int) * 5);
  if (strlen(input) != 40) {
    puts("Invalid input");
    return;
  }
  int i;
  for (i = 0; i < 5; i++)
    target[i] = 0;
  for (i = 0; i < 40; i = i + 4) {
    char buffer[5] = {0};
    memcpy(buffer, input + i, 4);
    target[i / 8] = target[i / 8] | (hex2int(buffer) << (1 - (i / 4) % 2) * 16);
  }

  int MAXDIGIT = 4;
  int digit;
  for (digit = 1; digit <= MAXDIGIT; digit++) {
    for (i = 0; i < digit; i++)
      inputBuffer[i] = 48;
    while (1) {
      int *out = sha1(inputBuffer, digit);
      if (array_equal(out, target, 5)) {
        for (i = 0; i < digit; i++)
          printf("%c", int2chr(inputBuffer[i]));
        puts("");
        return;
      }
      if (!nextText(inputBuffer, digit))
        break;
    }
  }
  printf("Not Found!\n");
}

int main() {
  initializeGlobalVariables();
  int op;
  char input[256];
  while (1) {
    scanf("%d", &op);
    if (op == 0)
      break;
    if (op == 1) {
      scanf("%s", input);
      computeSHA1(input);
    } else if (op == 2) {
      scanf("%s", input);
      crackSHA1(input);
    }
  }
  return 0;
}
