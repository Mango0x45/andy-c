package main

import (
	"fmt"
	"strings"
	"unicode"
)

type Range struct {
	lo, hi rune
}

func main() {
	MkFunc("unispace", unicode.IsSpace)
}

func MkFunc(name string, pred func(rune) bool) {
	fmt.Printf("bool %s(rune ch) { return ", name)

	xs := []string{}
	for ch := rune(0); ch < unicode.MaxRune; ch++ {
		if !pred(ch) {
			continue
		}
		lo := ch
		for pred(ch) {
			ch++
		}
		xs = append(xs, Range{lo, ch - 1}.String())
	}

	fmt.Print(strings.Join(xs, " || "))
	fmt.Println("; }")
}

func (r Range) String() string {
	switch r.hi - r.lo {
	case 0:
		return fmt.Sprintf("ch == 0x%04X", r.lo)
	case 1:
		return fmt.Sprintf("ch == 0x%04X || ch == 0x%04X", r.lo, r.hi)
	}
	return fmt.Sprintf("(ch >= 0x%04X && ch <= 0x%04X)", r.lo, r.hi)
}
