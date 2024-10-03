if grep -q \"$(realpath $2)\" /proc/$(pidof $1)/maps; then
  exit
fi

gdb -n -q -batch -ex "attach $(pidof $1)" \
  -ex "set \$dlopen = (void*(*)(char*, int)) dlopen" \
  -ex "set \$library = \$dlopen(\"$(realpath $2)\", 6)" \
  -ex "set \$dlclose = (int(*)(void*)) dlclose" \
  -ex "call \$dlclose(\$library)" \
  -ex "detach" \
  -ex "quit"