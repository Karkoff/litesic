set -e
export LITESIC_NO_CAPTURE=1
for arch in 64 32; do
 for debug in 0 1; do
  config=release
  if [ "$debug" = 1 ]; then config=debug; fi
  make -B ARCH=$arch DEBUG=$debug all $(for f in tests/*.c tests/*.cpp; do name=${f##*/}; name=${name%.*}; printf 'build/%s/%s/test-%s.exe ' "$arch" "$config" "$name"; done) > build/audit-m6/final-$arch-$config-build.log 2>&1
  make ARCH=$arch DEBUG=$debug test > build/audit-m6/final-$arch-$config-test.log 2>&1
  echo "PASS $arch $config build and suite"
 done
done
make ARCH=64 DEBUG=0 build/64/release/eq-probe.exe > build/audit-m6/probe-build.log 2>&1
make ARCH=32 DEBUG=0 build/32/release/eq-probe.exe >> build/audit-m6/probe-build.log 2>&1
