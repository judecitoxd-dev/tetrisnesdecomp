-- Tetris NES v0.11 reference trace for Mesen 2 / Mesen CE.
local folder = emu.getScriptDataFolder()
if folder == nil or folder == "" then error("Mesen script data folder unavailable") end

local trace = assert(io.open(folder .. "/tetris-reference.csv", "w"))
local apu = assert(io.open(folder .. "/tetris-apu-writes.csv", "w"))
local frame = 0
local frameWrites = {}

local function read8(address)
    return emu.read(address, emu.memType.nesDebug, false)
end

local function signed8(address)
    local value = read8(address)
    if value >= 0x80 then return value - 0x100 end
    return value
end

local function read16(address)
    return read8(address) | (read8(address + 1) << 8)
end

local function bcd3(address)
    return read8(address) + (read8(address + 1) << 8) + (read8(address + 2) << 16)
end

local function demoInputMask()
    local buttons = read8(0x00CE)
    local mask = 0
    if (buttons & 0x02) ~= 0 then mask = mask | 0x001 end
    if (buttons & 0x01) ~= 0 then mask = mask | 0x002 end
    if (buttons & 0x04) ~= 0 then mask = mask | 0x004 end
    if (buttons & 0x80) ~= 0 then mask = mask | 0x008 end
    if (buttons & 0x40) ~= 0 then mask = mask | 0x010 end
    if (buttons & 0x10) ~= 0 then mask = mask | 0x040 end
    return mask
end

local function fnv1a32(startAddress, count)
    local hash = 0x811C9DC5
    for offset = 0, count - 1 do
        hash = ((hash ~ read8(startAddress + offset)) * 0x01000193) & 0xFFFFFFFF
    end
    return hash
end

trace:write("frame,input,x,y,orientation_id,next_orientation_id,level,fall_counter,das_counter,play_state,lines_bcd,score_bcd,rng_seed,frame_counter,spawn_count,game_mode,music_track,playfield_hash32,apu_writes\n")
apu:write("frame,apu_writes\n")

local function onApuWrite(address, value)
    frameWrites[#frameWrites + 1] = string.format("%04X=%02X", address, value)
end

local function onEndFrame()
    local writes = table.concat(frameWrites, "|")
    trace:write(string.format(
        "%d,%03X,%d,%d,%d,%d,%d,%d,%d,%d,%06X,%06X,%04X,%d,%d,%d,%d,%08X,%s\n",
        frame, demoInputMask(), signed8(0x0040), signed8(0x0041),
        read8(0x0042), read8(0x00BF), read8(0x0044), read8(0x0045),
        read8(0x0046), read8(0x0048), bcd3(0x0050), bcd3(0x0053),
        read16(0x0017), read16(0x00B1), read8(0x001A), read8(0x00C0),
        read8(0x06F5), fnv1a32(0x0400, 200), writes))
    apu:write(string.format("%d,%s\n", frame, writes))
    trace:flush()
    apu:flush()
    frameWrites = {}
    frame = frame + 1
end

local function closeFiles()
    if trace then trace:close(); trace = nil end
    if apu then apu:close(); apu = nil end
end

emu.addMemoryCallback(onApuWrite, emu.callbackType.write,
    0x4000, 0x4017, emu.cpuType.nes, emu.memType.nesMemory)
emu.addEventCallback(onEndFrame, emu.eventType.endFrame)
emu.addEventCallback(closeFiles, emu.eventType.scriptEnded)
