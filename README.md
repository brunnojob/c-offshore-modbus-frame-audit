# c-offshore-modbus-frame-audit

Validate captured Modbus RTU frames, CRC integrity, exception responses and register decoding.

```sh
make test
./app < examples/input.txt
```

Offline diagnostic tool. Inputs and limits must be validated for each installation; no equipment commands are sent.

MIT. Built by [brunnodev](https://brunnodev.store).
