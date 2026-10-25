# The flash map, and the decision P08 forced on it

Written Sunday 25 October 2026, before any build of P07 or P08, because the refactor of
Tuesday 20 October 2026 read the bootloader's design document against the map chapter 07 drew
and found that no swap mode can run on it. This page carries the part's real geometry, the
arithmetic for each option, one map that serves both of P08's arms, and what in chapter 07 the
geometry corrects. Nothing here is measured; every number is read from a named source or
computed from one, and the chapters change only when the decision at the end is taken.

## The part, from the tree this volume pins

The kernel's own description of the STM32H7A3, `dts/arm/st/h7/stm32h7a3.dtsi`, read on Sunday
25 October 2026:

| Property | Value | What it decides |
|---|---|---|
| flash base | `0x0800 0000` | every offset below is from here |
| flash size | 2048 kB, two banks of 1024 kB | the bank boundary is at offset `0x10 0000` |
| `erase-block-size` | 8 kB | one flash sector; there are 256 of them |
| `write-block-size` | 16 bytes | the bootloader's minimum write, which sizes its swap status |
| `max-erase-time` | 3 ms per sector | the longest window a power cut can land in, per sector |

The board's own description, `boards/st/nucleo_h7a3zi_q/nucleo_h7a3zi_q.dts`, defines no
partitions node, which is the finding chapter 07 opens with and is confirmed rather than assumed.

**The sibling this part is mistaken for has 128 kB sectors.** Chapter 07's own pitfall warns
against taking sector geometry from material about the popular sibling, and the 8 kB figure
above is why the warning exists: a layout that is sixteen sectors on this part is one sector on
the other, and every erase-time figure differs by the same factor.

## What the bootloader requires of a map

From the bootloader's design document and its Zephyr readme, both read on Tuesday 20 October
2026, and its Kconfig read on Sunday 25 October 2026:

- **Swap using scratch** needs a third partition, `scratch_partition`, at least as large as the
  largest sector of either slot and a multiple of the sector size. The Zephyr readme says this
  mode "is not recommended" and names swap using offset or swap using move instead. Every
  sector of the image is moved three times per swap.
- **Swap using move** needs the primary slot "larger than the secondary slot by exactly one
  sector plus the size of the swap status area, rounded up to the total size of the sectors it
  occupies". It erases the primary slot twice and the secondary once per swap. It needs uniform
  sector sizes, which this part has.
- **Swap using offset** puts the spare sector in the secondary slot instead and uses two status
  records per sector rather than three. It is the Kconfig default when the preference symbol is
  set.
- The swap status area is `BOOT_MAX_IMG_SECTORS` times the minimum write size times the number
  of records per sector. `BOOT_MAX_IMG_SECTORS` defaults to 128 and must cover the larger slot.
- The two image slots must be contiguous.

On this part, with the default of 128 sectors and a 16-byte write block:

| Mode | Status records per sector | Swap status area | Trailer, with its five 16-byte fields | Sectors the trailer occupies |
|---|---|---|---|---|
| scratch, move | 3 | 6144 B | 6224 B | 1 |
| offset | 2 | 4096 B | 4176 B | 1 |

So under swap using move the primary slot must be exactly **two sectors, 16 kB,** larger than
the secondary: one sector for the move, one for the status area rounded up.

## The map chapter 07 drew, and why it cannot swap

| Partition | Offset | Size | Sectors |
|---|---|---|---|
| boot | `0x00 0000` | 128 kB | 16 |
| slot 0 | `0x02 0000` | 896 kB | 112 |
| slot 1 | `0x10 0000` | 896 kB | 112 |
| storage | `0x1E 0000` | 128 kB | 16 |

Two equal slots and no scratch partition: scratch has no partition, move and offset have no
spare sector. The map is internally consistent and the chapter's argument for it, that storage
sits above both slots, is right. It is the one row the bootloader reads that is missing.

## The decision, as two options and one recommendation

**Option A, one map that serves both of P08's arms.** The bootloader's partition shrinks by two
sectors, which its budget row (under 64 kB of 128) allows four times over; the primary slot
takes those two sectors; the storage partition gives half of itself to a scratch area. Slot 1
stays exactly at the bank boundary and the storage partition stays above both slots, so chapter
07's argument is untouched.

| Partition | Offset | Size | Sectors | Change |
|---|---|---|---|---|
| boot | `0x00 0000` | 112 kB | 14 | two sectors fewer |
| slot 0 | `0x01 C000` | 912 kB | 114 | two sectors more: move's requirement exactly |
| slot 1 | `0x10 0000` | 896 kB | 112 | unchanged |
| scratch | `0x1E 0000` | 64 kB | 8 | new |
| storage | `0x1F 0000` | 64 kB | 8 | half |

Sum: 14 + 114 + 112 + 8 + 8 = 256 sectors, 2048 kB. Under move the largest image is
(114 minus 1) times 8 kB minus the one trailer sector, 896 kB; under scratch it is the secondary
slot minus its trailer, 888 kB; the application budget is under 400 kB. Eight scratch sectors
rather than one means about seven scratch erases per swap of a 400 kB image instead of fifty,
which is the design document's own argument for a larger scratch. Sixty-four kB of storage is
two store sectors of 32 kB, which is eight flash sectors each and still two, against a settings
budget under 512 bytes and a credential under 2 kB in P13; for the filesystem arm of P07 it is
eight blocks of 8 kB, four times the filesystem's minimum.

This is the recommendation, for one reason that is about the measurement rather than the
layout: P08's replicated claim times the swap under scratch and under move with the same image,
and a control must not move the subject. One map, two modes, one image, and the mode is the
only thing that changes between the two runs.

**Option B, move only, storage untouched.** Boot 112 kB, slot 0 912 kB, slot 1 896 kB, storage
128 kB. Smaller change, one arm. The scratch timing in P08's claim would then read not built,
and the variants row that names scratch as the mode drawn would be describing a mode the map
cannot run.

**Not an option: taking the sectors from slot 1.** Slot 1 would then start at `0x10 4000`,
inside the second bank rather than at its boundary, for no gain.

The overlay for option A, written so it can be read against the table and not the other way
round:

```dts
&flash0 {
    partitions {
        compatible = "fixed-partitions";
        #address-cells = <1>;
        #size-cells = <1>;

        boot_partition:    partition@0      { label = "mcuboot";
                                              reg = <0x00000000 0x0001c000>; };
        slot0_partition:   partition@1c000  { label = "image-0";
                                              reg = <0x0001c000 0x000e4000>; };
        slot1_partition:   partition@100000 { label = "image-1";
                                              reg = <0x00100000 0x000e0000>; };
        scratch_partition: partition@1e0000 { label = "image-scratch";
                                              reg = <0x001e0000 0x00010000>; };
        storage_partition: partition@1f0000 { label = "storage";
                                              reg = <0x001f0000 0x00010000>; };
    };
};
```

## What the geometry corrects in chapter 07, whichever option is taken

Three things in the chapter were written as if a sector were larger than it is, and each is a
prediction rather than a measurement, so each is corrected here before the run that would have
found it.

1. **"Two sectors" means two store sectors, not two flash sectors.** The 128 kB partition is
   sixteen flash sectors; the store's sector is a multiple of the flash page and the chapter
   chose 64 kB. The text, the architecture figure's label and the budget row all say "two
   sectors" without saying whose. They should say two store sectors, and give the flash count.
2. **The erase window is milliseconds, not hundreds of them.** The tree gives 3 ms as the
   maximum erase time of one 8 kB sector, so a 64 kB store sector erases in at most 24 ms and a
   32 kB one in at most 12 ms. The budget row says under 400 ms, which is the order of a 128 kB
   sector on the sibling part. The harness's argument stands, an erase is still two to three
   orders longer than one 16-byte write, but the window it aims a cut at is shorter by more
   than an order of magnitude and the aim has to be read from this number.
3. **The write block is 16 bytes.** The store's entries and the bootloader's trailer are both
   sized by it; the chapter's record structure of 16 bytes fits one write block exactly, which
   is worth saying because a record of 17 bytes would cost two.

These become chapter edits in the commit that takes the decision, and in no other.

## Sources

- The kernel's `dts/arm/st/h7/stm32h7a3.dtsi` and `boards/st/nucleo_h7a3zi_q/nucleo_h7a3zi_q.dts`
  at the tree this volume pins, read on Sunday 25 October 2026.
- MCUboot's `docs/design.md`, read on Tuesday 20 October 2026 for the slot requirements of each
  mode, the swap status formula and the trailer; its `docs/readme-zephyr.md` for the partition
  labels and the recommendation against scratch; its `boot/zephyr/Kconfig` on Sunday 25 October
  2026 for `BOOT_MAX_IMG_SECTORS` and the mode choice.
- Chapter 07 of this volume for the map as drawn, and chapter 08 for the claim this decision
  unblocks.
