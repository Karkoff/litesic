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
