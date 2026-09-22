# compilador-macarronica-front

### Nada de commitar na `main` direto, tô de olho 👀

Fluxo do projeto:

```text
main
  ↑
develop
  ↑
branch-dev
```

Ou seja:

```text
main <- develop <- branch-de-trabalho
```

## Para trabalhar

Sempre puxe a `develop` antes de começar:

```bash
git checkout develop
git pull origin develop

git checkout branch-dev
git merge develop
```

Trabalhe e faça os commits na sua `branch-dev`:

```bash
git add .
git commit -m "alterações"
git push origin branch-dev
```

Quando terminar, mande sua branch para a `develop`:

```bash
git checkout develop
git pull origin develop
git merge branch-dev
git push origin develop
```

Quando a `develop` estiver testada e funcionando:

```bash
git checkout main
git pull origin main
git merge develop
git push origin main
```

### Resumindo

```text
develop -> branch-de-trabalho -> develop -> main
```

**Nunca desenvolver ou commitar diretamente na `main`.**

## Documentação

| Documento | Conteúdo |
|---|---|
| [`docs/guia-defesa-autoria.md`](docs/guia-defesa-autoria.md) | Guia para compreender e apresentar o front-end. |
| [`docs/diagnosticos.md`](docs/diagnosticos.md) | Catálogo dos códigos de erro léxico, sintático e semântico. |
