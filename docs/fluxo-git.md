# Fluxo de branches do grupo

Ordem de integração:

```text
develop -> branch-de-trabalho -> develop -> main
```

**Nunca desenvolver ou commitar diretamente na `main`.** Cada integrante
trabalha numa branch própria criada a partir da `develop` e integra por pull
request na `develop`; a `main` só recebe a `develop` revisada.
