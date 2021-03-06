#!/usr/bin/env python

from __future__ import print_function

import fumen
import mmap

import struct
import time

import pyperclip

def enum(**enums):
    return type('Enum', (), enums)

TapState = enum(
    NONE=0,
    Start=1,
    Active=2,
    Locking=3,     # Cannot be influenced anymore
    Lineclear=4,   # Tetromino is being locked to the playfield.
    Entry=5,
    Gameover=7,    # "Game Over" is being shown on screen.
    Idle=10,       # No game has started just waiting...
    Fading=11,     # Blocks fading away when topping out (losing).
    Completion=13, # Blocks fading when completing the game
    Startup=71
)

TapMRollFlags = enum(
    M_FAIL_1   = 17,
    M_FAIL_2   = 19,
    M_FAIL_END = 31,

    M_NEUTRAL  = 48,
    M_PASS_1   = 49,
    M_PASS_2   = 51,
    M_SUCCESS  = 127,
)

DATA_BLOCK_SIZE = 4

# TGM2+ indexes its pieces slightly differently to fumen, so when encoding a
# diagram we gotta convert the index.
# 2 3 4 5 6 7 8 (TAP)
# I Z S J L O T
# 1 4 7 6 2 3 5 (Fumen)
TapToFumenMapping = [0, 0, 1, 4, 7, 6, 2, 3, 5]

def calculateFumenOffset(block, rotation):
    """Given a fumen tetromino index and rotation state, output a tuple (x, y) that
    represents how far to offset TGM2+'s (x, y) location coordinate."""
    if block == 1:
        if rotation == 1 or rotation == 3:
            return (1, 0)
    elif block == 6:
        if rotation == 2:
            return (0, -1)
    elif block == 2:
        if rotation == 2:
            return (0, -1)
    elif block == 5:
        if rotation == 2:
            return (0, -1)
    return (0, 0)

def inPlayingState(state):
    """Given the game's current state, determine whether or not we're in game."""
    return state != TapState.NONE and state != TapState.Idle and state != TapState.Startup

def testMasterConditions(flags):
    """Given TGM2+'s M-Roll flags, return true if all the requirements so far have
been met. If any condition has failed, return false."""
    return flags == TapMRollFlags.M_NEUTRAL or flags == TapMRollFlags.M_PASS_1 or flags == TapMRollFlags.M_PASS_2 or flags == TapMRollFlags.M_SUCCESS

def unpack_mmap_block(mm, n):
    """Decode the nth 4-byte long byte string from mapped memory."""
    return struct.unpack("<L", mm[n*DATA_BLOCK_SIZE:(n+1)*DATA_BLOCK_SIZE])[0]

def main():
    with open("/dev/shm/tgm2p_data", "r+b") as f:
        vSize = DATA_BLOCK_SIZE * 13
        mm = mmap.mmap(f.fileno(), vSize)

        frameList = []
        frame = fumen.Frame()

        creditReset = False

        prevState = state = 0
        prevLevel = level = 0
        prevGametime = gametime = 0
        while True:
            # We want to detect /changes/ in game state, so we should keep track
            # of the previous game state...
            prevState = state

            # At the end of a game, we'll be clearing level and time data, we
            # should store a copy of it for output.
            prevLevel = level
            prevGametime = gametime

            state = unpack_mmap_block(mm, 0)
            level = unpack_mmap_block(mm, 1)
            gametime = unpack_mmap_block(mm, 2)
            # mrollFlags = unpack_mmap_block(mm, 5)
            inCreditRoll = unpack_mmap_block(mm, 6)
            currentBlock = TapToFumenMapping[unpack_mmap_block(mm, 8)]
            currentX = unpack_mmap_block(mm, 10)
            currentY = unpack_mmap_block(mm, 11)
            rotState = unpack_mmap_block(mm, 12)

            # When inspecting the game's memory, I found that currentX
            # underflows for the I tetromino, so let's "fix" that.
            if currentX > 10:
                currentX = -1

            # Coordinates from TAP do not perfectly align with fumen's
            # coordinates.
            offsetX, offsetY = calculateFumenOffset(currentBlock, rotState)

            # Set the current frame's tetromino + location
            frame.willlock = True
            frame.piece.kind = currentBlock
            frame.piece.rot = rotState
            frame.piece.setPosition(currentX + offsetX, currentY + offsetY)

            # If we've entered the M-Roll, clear the field. This doesn't test
            # for a specific mode yet, only if the M-Roll conditions have been
            # met.
            if not creditReset and inCreditRoll:
                frameList.append(frame.copy())
                frame = frame.next()
                frame = fumen.Frame()
                creditReset = True

            # If a piece is locked in...
            if inPlayingState(state) and prevState == TapState.Active and state == TapState.Locking:
                frameList.append(frame.copy())
                frame = frame.next()

            # If the game is over...
            if inPlayingState(prevState) and not inPlayingState(state):
                # Lock the previous piece and place the killing piece. It's
                # state is not set, so the above lock check will not be run.
                frameList.append(frame.copy())
                frame = frame.next()

                fumenURL = fumen.make(frameList, 0)
                print("level %03d @ %02d:%02d\n%s\n" % (prevLevel, prevGametime / 60 / 60, prevGametime / 60 % 60, fumenURL))
                pyperclip.copy(fumenURL)

                frameList = []
                frame = fumen.Frame()
                creditReset = False

            time.sleep(0.01)
        mm.close()

if __name__ == '__main__':
    main()
