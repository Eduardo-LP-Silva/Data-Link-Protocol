#ifndef UTILITIES_H
#define UTILITIES_H

void swap(char* a, char*b);
int abs(int a);
void shiftRight(char* buffer, int size, int position, int shift);
void shiftLeft(char* buffer, int size, int position, int shift);
void printArray(char* arr, int length);
int stateMachine(char received[], int C);
int llopen(int fd, int flag);

#endif