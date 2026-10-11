set pagination off
set confirm off
break src/app.cpp:95
run --validation
print validationDirectory
print g_visualizerVisible
print g_userScalePercent
print Eq::State()
print g_audio.active
print g_eqActive
print g_eq.rate_
print g_eq.banks_[0].snapshot.enabled
call (long long)SendMessageW(hwnd,0x10,0,0)
continue
quit

