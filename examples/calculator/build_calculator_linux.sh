set -euo pipefail

CC="${CC:-cc}"
CFLAGS="-std=c89 -Wall -pedantic -Wextra -Werror"
${CC} ${CFLAGS} -shared -fPIC -g -o calculator.so -I ../../ calculator.c ../../eri_sdk.c ../../eri_sdk_linux.c
