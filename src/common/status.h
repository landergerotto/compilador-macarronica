#ifndef MACARONICA_COMMON_STATUS_H
#define MACARONICA_COMMON_STATUS_H

/*
 * Todas as fases usam o mesmo tipo de retorno. Assim da pra separar um erro no
 * codigo Macaronica de um erro do nosso proprio compilador.
 */
typedef enum {
    FRONTEND_STATUS_OK,             /* A fase terminou normal. */
    FRONTEND_STATUS_SOURCE_ERROR,   /* O problema esta no arquivo .mac. */
    FRONTEND_STATUS_INTERNAL_ERROR  /* O problema esta no compilador. */
} FrontendStatus;

#endif
