:SHA-1: 73fa2e6a9c01f70c09fd2ed7eb965ee59b924580 - StrongDC At revision: 13665.

: Basis for comparison
set basis-cmp-StrongDC=73fa2e6a9c01f70c09fd2ed7eb965ee59b924580

: prefix changes
set prefix-ident=%DATE% %TIME%
FOR /F %%i IN ('git rev-parse HEAD') DO set git-cur-sha1=%%i

set ident-content=[diff: StrongDC++ sqlite][ %prefix-ident% :: compared with SHA-1: %git-cur-sha1% ]


: Files
echo %ident-content% > CHANGES-StrongDC


: StrongDC project
git diff --diff-filter=DMRTUXB --no-prefix %basis-cmp-StrongDC% >> CHANGES-StrongDC