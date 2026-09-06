.global kmain
kmain:
    # Print 'Nux!' to VGA Buffer
    mov $0xb8000, %edi
    movb $0x4E, (%edi)
    movb $0x0A, 1(%edi)
    movb $0x75, 2(%edi)
    movb $0x0A, 3(%edi)
    movb $0x78, 4(%edi)
    movb $0x0A, 5(%edi)
    movb $0x21, 6(%edi)
    movb $0x0A, 7(%edi)
    ret
