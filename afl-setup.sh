#!/bin/bash

if [[ -e build-afl ]]; then
    echo "!! build-afl directory exists, exiting"
    exit 1
fi

echo ">> creating build-afl directory"
mkdir build-afl
pushd build-afl >/dev/null
echo ">> generating build system with CMake"
cmake ..

echo ">> patching afl-gcc into loader Makefiles"
pushd loader/CMakeFiles/loader.dir/ >/dev/null
sed -i 's|/usr/bin/cc|afl-gcc|g' build.make
sed -i 's|/usr/bin/cc|afl-gcc|g' link.txt
popd >/dev/null

echo ">> building loader"
make

echo ">> creating fuzzer directories"
mkdir afl.in
mkdir afl.out

echo ">> creating fuzzer testcase file"
touch /tmp/afl.elf
chmod +x /tmp/afl.elf

echo ">> copying testcases to fuzzer input directory"
for f in tests/bin/hello*; do
    cp "$f" afl.in/
done

echo ">> setting up system for afl"
echo "core" | sudo tee /proc/sys/kernel/core_pattern

echo ">> creating afl fuzzer script"
cat <<EOF >fuzz.sh
#!/bin/bash
LD_NOEXEC=1 AFL_SKIP_CPUFREQ=1 afl-fuzz -i afl.in/ -o afl.out/ -f /tmp/afl.elf -- ./loader/libloader.so /tmp/afl.elf
EOF
chmod +x fuzz.sh

echo ">> starting afl"
./fuzz.sh
