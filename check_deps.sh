#!/bin/bash
echo "=== Analisi dipendenze piHPSDR.exe e DLL ==="
echo ""

DIST="dist-windows"
MINGW_BIN="/usr/x86_64-w64-mingw32/bin"
GCC_LIB="/usr/lib/gcc/x86_64-w64-mingw32/10-win32"

# Raccogli tutte le dipendenze
ALL_DEPS=$(for f in "$DIST"/*.dll "$DIST"/pihpsdr.exe; do 
    x86_64-w64-mingw32-objdump -p "$f" 2>/dev/null | grep "DLL Name:" | awk '{print $3}'
done | sort -u)

echo "Dipendenze totali richieste: $(echo "$ALL_DEPS" | wc -l)"
echo ""

# Separa system DLLs da quelle che dobbiamo fornire
echo "=== DLL di sistema Windows (OK) ==="
for dll in $ALL_DEPS; do
    case $dll in
        KERNEL32.dll|USER32.dll|ADVAPI32.dll|msvcrt.dll|GDI32.dll|SHELL32.dll|ole32.dll|OLEAUT32.dll|WS2_32.dll|WSOCK32.dll|COMDLG32.dll|IMM32.dll|COMCTL32.dll|VERSION.dll|WINMM.dll|CRYPT32.dll|IPHLPAPI.dll|AVRT.dll|SAPI.dll|uuid.dll|RPCRT4.dll|SETUPAPI.dll|bcrypt.dll|ntdll.dll)
            echo "  ✓ $dll"
            ;;
    esac
done

echo ""
echo "=== DLL fornite nel pacchetto ==="
for dll in $ALL_DEPS; do
    case $dll in
        KERNEL32.dll|USER32.dll|ADVAPI32.dll|msvcrt.dll|GDI32.dll|SHELL32.dll|ole32.dll|OLEAUT32.dll|WS2_32.dll|WSOCK32.dll|COMDLG32.dll|IMM32.dll|COMCTL32.dll|VERSION.dll|WINMM.dll|CRYPT32.dll|IPHLPAPI.dll|AVRT.dll|SAPI.dll|uuid.dll|RPCRT4.dll|SETUPAPI.dll|bcrypt.dll|ntdll.dll)
            ;; # Skip
        *)
            if [ -f "$DIST/$dll" ]; then
                echo "  ✓ $dll"
            fi
            ;;
    esac
done

echo ""
echo "=== ⚠️  DLL MANCANTI nel pacchetto ==="
MISSING=0
for dll in $ALL_DEPS; do
    case $dll in
        KERNEL32.dll|USER32.dll|ADVAPI32.dll|msvcrt.dll|GDI32.dll|SHELL32.dll|ole32.dll|OLEAUT32.dll|WS2_32.dll|WSOCK32.dll|COMDLG32.dll|IMM32.dll|COMCTL32.dll|VERSION.dll|WINMM.dll|CRYPT32.dll|IPHLPAPI.dll|AVRT.dll|SAPI.dll|uuid.dll|RPCRT4.dll|SETUPAPI.dll|bcrypt.dll|ntdll.dll)
            ;; # Skip system DLLs
        *)
            if [ ! -f "$DIST/$dll" ]; then
                MISSING=$((MISSING + 1))
                # Cerca dove si trova
                if [ -f "$MINGW_BIN/$dll" ]; then
                    echo "  ❌ $dll (trovata in $MINGW_BIN)"
                elif [ -f "$GCC_LIB/$dll" ]; then
                    echo "  ❌ $dll (trovata in $GCC_LIB)"
                else
                    echo "  ❌ $dll (NON TROVATA)"
                fi
            fi
            ;;
    esac
done

echo ""
if [ $MISSING -eq 0 ]; then
    echo "✅ Tutte le DLL necessarie sono incluse!"
else
    echo "⚠️  Mancano $MISSING DLL nel pacchetto"
fi
