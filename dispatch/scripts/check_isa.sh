#!/bin/bash
# Checks, with objdump, that the SIMD neutral binaries of the AFF3CT dispatch build (launcher, host library, test,
# StreamPU) contain no instruction beyond the x86-64 baseline (SSE2), and that each plugin contains only the
# instructions of its variant.
#
# usage: dispatch/scripts/check_isa.sh <build_dir>

set -u
BUILD=${1:-build}

# x86-64 baseline: no SSE3/SSSE3/SSE4/POPCNT/BMI/AVX/AVX-512
SSE3PLUS='^(addsubp[sd]|hadd|hsub|lddqu|movddup|movs[hl]dup|pshufb|palignr|phadd|phsub|pmaddubsw|pmulhrsw|pabs[bwd]|psign[bwd]|ptest|pblendvb|pblendw|pmin(s[bd]|u[wd])|pmax(s[bd]|u[wd])|pmov[sz]x|pmuldq|pmulld|packusdw|pcmpeqq|pcmpgtq|pextr[bdq]|pinsr[bdq]|round[ps][sd]|dpp[sd]|insertps|extractps|blendv?p[sd]|mpsadbw|phminposuw|movntdqa|crc32|popcnt|pcmp[ei]str[im]|lzcnt|tzcnt|andn|bextr|blsi|blsr|blsmsk|bzhi|pdep|pext|mulx|rorx|sarx|shlx|shrx|movbe)$'

# prints "<n VEX/EVEX instructions> <n ymm> <n zmm> <n opmask> <n SSE3+ (non VEX)>" for a binary
count() {
    objdump -d --no-show-raw-insn "$1" | awk -v sse="$SSE3PLUS" '
        /^ *[0-9a-f]+:\t/ {
            split($0, f, "\t"); ins = f[2]; split(ins, w, " "); m = w[1];
            if (m ~ /^v/ && m !~ /^(verr|verw|vmcall|vmlaunch|vmresume|vmxoff|vmxon|vmread|vmwrite|vmptr|vmclear|vmfunc)/) vex++;
            if (ins ~ /%ymm/) ymm++;
            if (ins ~ /%zmm/) zmm++;
            if (ins ~ /%k[0-7]/) kreg++;
            if (m ~ sse) s3++;
        }
        END { printf "%d %d %d %d %d\n", vex, ymm, zmm, kreg, s3 }'
}

status=0
check() { # <file> <max_vex> <max_ymm> <max_zmm> <max_k> <max_sse3plus> (-1 = any)
    local f=$1; shift
    read -r vex ymm zmm k s3 < <(count "$f")
    local verdict=OK i=0
    local vals=($vex $ymm $zmm $k $s3) maxs=("$@")
    for i in 0 1 2 3 4; do
        if [ "${maxs[$i]}" -ge 0 ] && [ "${vals[$i]}" -gt "${maxs[$i]}" ]; then verdict=FAIL; status=1; fi
    done
    printf "%-60s VEX/EVEX=%-7s ymm=%-7s zmm=%-7s k=%-6s SSE3+=%-6s %s\n" "$(basename "$f")" "$vex" "$ymm" "$zmm" "$k" "$s3" "$verdict"
}

echo "== SIMD neutral binaries (expected: nothing beyond SSE2)"
for f in "$BUILD"/bin/aff3ct-[0-9]* "$BUILD"/bin/test-dispatch-turbo "$BUILD"/lib/libaff3ct-dispatch-*.so "$BUILD"/lib/streampu/lib/libstreampu.so; do
    [ -f "$f" ] && check "$f" 0 0 0 0 0
done
echo "== plugins"
for f in "$BUILD"/lib/libaff3ct-*-*.so; do
    case "$(basename "$f")" in
        *dispatch*) ;;
        *-sse4.2.so) check "$f" 0 0 0 0 -1 ;;   # SSE4.2 allowed, no VEX at all
        *-sse2.so)   check "$f" 0 0 0 0 0 ;;
        *-avx2.so)   check "$f" -1 -1 0 0 -1 ;; # VEX and ymm allowed, no EVEX (zmm, opmasks)
        *-avx512.so) check "$f" -1 -1 -1 -1 -1 ;;
    esac
done
exit $status
