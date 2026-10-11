set -e
export LITESIC_NO_CAPTURE=1
for arch in 64 32; do
 for debug in 0 1; do
  config=release
  if [ "$debug" = 1 ]; then config=debug; fi
  make ARCH=$arch DEBUG=$debug all >> build/audit-m6/final-$arch-$config-build.log 2>&1
  make ARCH=$arch DEBUG=$debug test > build/audit-m6/final-$arch-$config-test.log 2>&1
  echo "FINAL PASS $arch $config"
 done
done
for arch in 64 32; do
 make ARCH=$arch DEBUG=0 build/$arch/release/eq-probe.exe >> build/audit-m6/probe-build.log 2>&1
 for round in 1 2 3; do
  for mode in 6 7 0 1 2 3 4 8 5 9 10; do
   build/$arch/release/eq-probe.exe $mode >> build/audit-m6/performance-$arch.log
  done
 done
 echo "PERFORMANCE PASS $arch"
done
build/64/release/eq-probe.exe 10 300 > build/audit-m6/soak-64.log
 echo "SOAK PASS"
