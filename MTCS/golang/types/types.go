package types

import (
	"bytes"
	"encoding/binary"
	"errors"
)

type Bytes []byte
type Uint8_t [1]byte
type Uint16_t [2]byte
type Uint32_t [4]byte
type Uint64_t [8]byte
type Uint128_t [16]byte

func BytesToUint(data []byte) (any, error) {
	switch len(data) {
	case 1:
		var u Uint8_t
		copy(u[:], data)
		return u, nil
	case 2:
		var u Uint16_t
		copy(u[:], data)
		return u, nil
	case 4:
		var u Uint32_t
		copy(u[:], data)
		return u, nil
	case 8:
		var u Uint64_t
		copy(u[:], data)
		return u, nil
	case 16:
		var u Uint128_t
		copy(u[:], data)
		return u, nil
	default:
		return nil, errors.New("not a supported uint type")
	}
}

func UintToBytes(data any) ([]byte, error) {
	switch v := data.(type) {
	case Uint8_t:
		return v[:], nil
	case Uint16_t:
		return v[:], nil
	case Uint32_t:
		return v[:], nil
	case Uint64_t:
		return v[:], nil
	case Uint128_t:
		return v[:], nil
	default:
		return nil, errors.New("not a supported uint type")
	}
}

func StructToBytes(data any) ([]byte, error) {
	buf := new(bytes.Buffer)
	err := binary.Write(buf, binary.LittleEndian, data)
	if err != nil {
		return nil, err
	}
	return buf.Bytes(), nil
}

func BytesToStruct(data []byte, out interface{}) error {
	if len(data) == 0 {
		return errors.New("empty data")
	}
	reader := bytes.NewReader(data)
	err := binary.Read(reader, binary.LittleEndian, out)
	if err != nil {
		return err
	}
	return nil
}
