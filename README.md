# HTMLPlay RS97 0.1 – ejecutor de minijuegos HTML para RetroFW

**Estado:** primera versión experimental. **Este IPK contiene un ejecutable MIPS32 real y dos juegos HTML de ejemplo.** Compilado y verificado en PC con una versión nativa de pruebas, pero **no probado en una RS97 física**. No es Chrome, Firefox ni un navegador web moderno.

## Instalar

1. Copiá `htmlplay_rs97_v0.1.ipk` a la tarjeta SD de la consola RS97 con RetroFW.
2. Abrilo desde Explorer de RetroFW para instalar.
3. En la sección **applications**, elegí **HTMLPlay RS97**.
4. Elegí un juego con la cruceta y presioná **A** o **Start**. **Select** regresa al menú y, estando en el menú, sale a RetroFW.

El ejecutable reside en `/home/retrofw/apps/htmlplay/htmlplay.dge`. Los HTML están en `/home/retrofw/apps/htmlplay/games/`. Para agregar un juego, copiá un archivo `.html` a ese directorio; reiniciá HTMLPlay para actualizar la lista.

**Requisitos previstos:** Linux MIPS little-endian o32 (RS97 RetroFW); `/dev/fb0` de 320×240 RGB565 con escritura normal y dispositivo(s) `/dev/input/event*`. La versión estática no depende de SDL ni uClibc ni requiere SDK instalado en la consola. **Si el hardware reporta otra resolución o no entrega los botones por evdev, puede no funcionar**; los códigos de teclas varían según revisión.

## Formato de juegos admitido

Archivos `.html` locales de hasta 128 KiB con `<canvas width="320" height="240">` y `<script>` JavaScript **embebido**. El intérprete soporta un **subconjunto** de JavaScript, pensado para juegos de movimiento, colisiones y puntuación, no JavaScript general. Cada cuadro (aprox. 25 FPS), se llaman automáticamente las funciones globales `update()` y `draw()`. Pueden usar variables globales `let`, `var`, `const`, números (precisión fija 1/256), strings, comparaciones, `if/else`, expresiones aritméticas, `&&`, `||`, `!` y los operadores `=`, `+=`, `-=`, `++`, `--`.

### API de dibujo

```javascript
const canvas = document.getElementById('game');
const ctx = canvas.getContext('2d');
let x = 30;
function update(){if(keys.ArrowRight) x += 2;}
function draw(){
  ctx.fillStyle='#071728';
  ctx.fillRect(0,0,320,240);
  ctx.fillStyle='#ffe17a';
  ctx.fillRect(x,100,20,20);
  ctx.fillText('HOLA RS97',10,10);
}
```

- `ctx.fillStyle = '#RRGGBB'` / `ctx.strokeStyle = '#RRGGBB'`; también colores hex cortos y algunos nombres simples.
- `ctx.fillRect(x,y,w,h)`, `clearRect`, `strokeRect`, `fillText(text,x,y)` (fuente pixel ASCII, origen superior izquierdo).
- `canvas.width`, `canvas.height`.
- `Math.random`, `Math.floor`, `Math.ceil`, `Math.round`, `Math.abs`, `Math.min`, `Math.max`, `Math.sign`.
- `keys.ArrowLeft`, `keys.ArrowRight`, `keys.ArrowUp`, `keys.ArrowDown`, `keys.Space` (A), `keys.x` (B), `keys.Enter` (Start), `keys.q` (L), `keys.e` (R).
- HTML: reconoce etiquetas `<canvas>`, `<style>`, `<script>`. CSS: admite `background` / `background-color` sencillo, colores sólidos. El resto de la página no tiene layout.

**No soporta:** APIs DOM completas, `addEventListener`, `requestAnimationFrame`, `setInterval`, `for`, `while`, arrays, objetos dinámicos, imágenes, archivos JS/CSS externos, audio, Internet, vídeo, fuentes web, transparencias, CSS moderno o funciones JS arbitrarias. Si querés portar un juego HTML existente, posiblemente haya que reescribirlo según esta API. El interprete NO aplica la semántica completa de ECMAScript.

## Controles

Cruceta: movimiento. **A**: `keys.Space`; **B**: `keys.x`; **Start**: `keys.Enter`; **L/R**: `keys.q/e`. **Select**: volver al selector de HTML; desde ahí cierra la aplicación.

## Compilar

En Linux con Clang, LLD y `file`:

```sh
cd source
./build.sh
```

Emite `../htmlplay.dge` como ejecutable **ELF MIPS32 little-endian o32 estático**, sin libc. Las pruebas nativas de PC: `./tests/test.sh`.

## Limitaciones de hardware

No se ha ejecutado en una RS97 física y no se puede asegurar que los controles y el framebuffer correspondan a todas las revisiones de RetroFW. Si aparece pantalla negra, comprobar `/dev/fb0` e input evdev. No sustituye un navegador con motor JavaScript real (Duktape/QuickJS) y CSS completo.

Código y glifos originales de este proyecto: licencia MIT (archivo `LICENSE`).

## Repositorio, CI y archivo IPK

- `source/htmlplay.c`: código del intérprete y del ejecutable RetroFW.
- `source/build.sh`: compilación estática de ELF MIPS32 little-endian.
- `games/`: dos juegos de muestra.
- `tests/test.sh`: pruebas de escritorio y verificación ELF MIPS32.
- `scripts/package-ipk.sh`: empaquetador RetroFW, con validaciones de arquitectura.
- `.github/workflows/build.yml`: prueba y generación automática del IPK como artifact de GitHub Actions.

La publicación en GitHub **no prueba compatibilidad con una RS97 física**.

Repositorio oficial: https://github.com/RealLotex/HTMLPlayRS97

