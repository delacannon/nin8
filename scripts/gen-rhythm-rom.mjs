// Extract YM2608_RHYTHM_ROM bytes from @opna/core Ym2608RhythmRom.ts → assets/ym2608_rhythm_rom.bin
import { readFileSync, writeFileSync, mkdirSync } from 'node:fs'
import { dirname, join } from 'node:path'
import { fileURLToPath } from 'node:url'

const root = dirname(dirname(fileURLToPath(import.meta.url)))
const src = readFileSync(join(root, '../opna/packages/core/src/Ym2608RhythmRom.ts'), 'utf8')

const m = src.match(/YM2608_RHYTHM_ROM = new Uint8Array\(\[([\s\S]*?)\]\)/)
if (!m) throw new Error('YM2608_RHYTHM_ROM array not found')

const bytes = m[1].split(',').map(s => s.trim()).filter(Boolean).map(s => {
  const v = Number(s)
  if (!Number.isInteger(v) || v < 0 || v > 255) throw new Error(`bad byte: ${s}`)
  return v
})

mkdirSync(join(root, 'assets'), { recursive: true })
writeFileSync(join(root, 'assets/ym2608_rhythm_rom.bin'), Buffer.from(bytes))
console.log(`wrote assets/ym2608_rhythm_rom.bin (${bytes.length} bytes)`)
