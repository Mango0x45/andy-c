package main

import (
	"fmt"
	"unicode"
)

var first = true

type Range struct {
	lo, hi rune
}

func main() {
	fmt.Print("bool unispace(rune_t ch) { return")

	rs := []Range{}
	for ch := rune(0); ch < unicode.MaxRune; ch++ {
		if !unicode.IsSpace(ch) {
			continue
		}
		lo := ch
		for unicode.IsSpace(ch) {
			ch++
		}
		rs = append(rs, Range{lo, ch - 1})
	}

	for _, r := range rs {
		f(r)
	}
	fmt.Println("; }")
}

func f(r Range) {
	if first {
		first = false
	} else {
		fmt.Print(" ||")
	}

	switch r.hi - r.lo {
	case 0:
		fmt.Printf(" ch == 0x%04X", r.lo)
	case 1:
		fmt.Printf(" ch == 0x%04X || ch == 0x%04X", r.lo, r.hi)
	default:
		fmt.Printf(" (ch >= 0x%04X && ch <= 0x%04X)", r.lo, r.hi)
	}
}
