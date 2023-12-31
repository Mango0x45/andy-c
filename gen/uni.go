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
		fmt.Print(r)
	}
	fmt.Println("; }")
}

func (r Range) String() string {
	var s string

	if first {
		first = false
	} else {
		s += " ||"
	}

	switch r.hi - r.lo {
	case 0:
		return s + fmt.Sprintf(" ch == 0x%04X", r.lo)
	case 1:
		return s + fmt.Sprintf(" ch == 0x%04X || ch == 0x%04X", r.lo, r.hi)
	}
	return s + fmt.Sprintf(" (ch >= 0x%04X && ch <= 0x%04X)", r.lo, r.hi)
}
