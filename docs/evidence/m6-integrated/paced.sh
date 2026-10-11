set -e
for mode in 7 0 1 2 3 4 8 5 9 10; do
 build/64/release/eq-probe.exe $mode 5 >> build/audit-m6/paced-64.log
done
