# verify_bijection

Program weryfikuje, że odwzorowanie `(b15, b16) → CRC16` jest bijekcją
dla każdego z 65 536 możliwych stanów startowych rejestru CRC-16/Modbus.

Opis wszystkich plików projektu: [OPIS_PLIKOW.txt](OPIS_PLIKOW.txt)

## Wymagania

- macOS
- clang++
- [just](https://github.com/casey/just)

## Budowanie i uruchamianie

```sh
just build   # kompilacja
just run     # kompilacja i uruchomienie
just clean   # usunięcie pliku wykonywalnego
```
