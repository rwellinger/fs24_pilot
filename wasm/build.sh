#!/usr/bin/env bash
# Build the fs24-pilot WASM standalone module against the MSFS 2024 SDK.
# Toolchain and flags copied from fs24_streamdeck/wasm/build.sh (proven to load).
#
#   TOOLCHAIN=sdk   Asobo's LLVM from the SDK (WASM/llvm, Windows only) — the
#                   release build; flags = SDK MSBuild toolset
#                   (WASM/vs/2022/Microsoft.Cpp.MSFS.Common.targets).
#   TOOLCHAIN=llvm  stock LLVM (Homebrew/apt) — Mac dev loop / compile check.
#                   Not loadable by the sim (fs24_streamdeck docs/research/wasm-bridge.md).
#
# Default: sdk when the SDK's clang-cl.exe is present and we run on Windows.
# Env: SDK_DIR (default ../sdk/current, relative to wasm/), LLVM_BIN, LLD_BIN.
set -euo pipefail

cd "$(dirname "$0")"
SDK_DIR="${SDK_DIR:-../sdk/current}"
[[ -f "$SDK_DIR/.complete" ]] || { echo "MSFS SDK missing in $SDK_DIR — run 'make sdk' first" >&2; exit 1; }

NAME=fs24-pilot
VERSION="$(tr -d '[:space:]' < VERSION)"
SYSROOT="$SDK_DIR/WASM/wasi-sysroot"
LIBDIR="$SYSROOT/lib/wasm32-wasi"
BUILD=build
mkdir -p "$BUILD/obj"

# Composition root + every translation unit of the inner rings.
# Domain/application sources are SDK-free; they are compiled for the host by CMake as well.
SOURCES=(src/module.cpp)
while IFS= read -r f; do SOURCES+=("$f"); done < <(find ../src -name '*.cpp' | sort)

if [[ -z "${TOOLCHAIN:-}" ]]; then
	TOOLCHAIN=llvm
	if [[ "$OSTYPE" == msys* || "$OSTYPE" == cygwin* ]] && [[ -x "$SDK_DIR/WASM/llvm/bin/clang-cl.exe" ]]; then
		TOOLCHAIN=sdk
	fi
fi

case "$TOOLCHAIN" in
sdk)
	# clang-cl in GNU driver mode accepts the same flags as clang++.
	CXX=("$SDK_DIR/WASM/llvm/bin/clang-cl.exe" --driver-mode=g++)
	WASM_LD=("$SDK_DIR/WASM/llvm/bin/wasm-ld.exe")
	TOOLCHAIN_LDFLAGS=(--stack-guard-page)
	;;
llvm)
	LLVM_BIN="${LLVM_BIN:-$( [[ -d /opt/homebrew/opt/llvm/bin ]] && echo /opt/homebrew/opt/llvm/bin || echo /usr/bin )}"
	LLD_BIN="${LLD_BIN:-$( [[ -d /opt/homebrew/opt/lld/bin ]] && echo /opt/homebrew/opt/lld/bin || echo /usr/bin )}"
	CXX=("$LLVM_BIN/clang++")
	WASM_LD=("$LLD_BIN/wasm-ld")
	# Stock lld lacks --stack-guard-page; newer clang defaults to more WASM
	# features than the sim accepts (MVP + bulk-memory only).
	TOOLCHAIN_LDFLAGS=(--stack-first -z stack-size=1048576)
	;;
*)
	echo "unknown TOOLCHAIN=$TOOLCHAIN" >&2
	exit 1
	;;
esac

CXXFLAGS=(
	--target=wasm32-unknown-wasi "--sysroot=$SYSROOT"
	-mcpu=mvp -mbulk-memory
	-std=c++20 -O3 -fstack-size-section -fvisibility=hidden
	-ffunction-sections -fdata-sections -fno-stack-protector -fno-exceptions
	-fms-extensions -fwritable-strings -Werror=return-type
	-Wall -Wno-deprecated -Wno-ignored-attributes -Wno-unused-function -Wno-unused-command-line-argument
	-D_MSFS_WASM "-DFS24_PILOT_VERSION=\"$VERSION\""
	-I../src
	"-I$SDK_DIR/WASM/include" "-I$SDK_DIR/SimConnect/include"
)

LDFLAGS=(
	--no-entry --allow-undefined --export-dynamic
	--export malloc --export free --export __wasm_call_ctors --export-table
	--export mallinfo --export mchunkit_begin --export mchunkit_next
	--export get_pages_state --export mark_decommit_pages
	--export GetSimConnectVersion --export GetExtensionVersion --export GetExtensionVersionBuffer
	"${TOOLCHAIN_LDFLAGS[@]}"
	--strip-debug --gc-sections -O3 "-L$LIBDIR"
)

LIBS=(
	-lc++ -lc++abi -lc "$LIBDIR/libclang_rt.builtins-wasm32.a" -lc
	"$SDK_DIR/WASM/WasmVersions/MSFS_WasmVersions.a"
)

OBJS=()
for src in "${SOURCES[@]}"; do
	obj="$BUILD/obj/$(echo "${src#../}" | tr '/' '_' | sed 's/\.cpp$/.o/')"
	echo "[$TOOLCHAIN] $src"
	"${CXX[@]}" "${CXXFLAGS[@]}" -c "$src" -o "$obj"
	OBJS+=("$obj")
done
"${WASM_LD[@]}" "${LDFLAGS[@]}" "${OBJS[@]}" "${LIBS[@]}" -o "$BUILD/$NAME.wasm"
echo "[$TOOLCHAIN] → wasm/$BUILD/$NAME.wasm ($VERSION)"
