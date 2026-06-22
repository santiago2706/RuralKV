#ifndef RURAL_SERVER_H
#define RURAL_SERVER_H

#include "arena.h"
#include "hash.h"
#include "wal.h"

// Función bloqueante: Abre el puerto TCP, parsea HTTP básico y responde.
void server_start(int port, HashTable* db, WAL* wal);

#endif
