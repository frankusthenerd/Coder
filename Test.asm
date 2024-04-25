Interrupt vector is three numbers. One is for the screen, input, and timer, respectively.
:label Interrupt_Vector
:list 3

:label Stack
:list 20

Screen size is (400 / 16) * (300 / 16).
:label Screen
:list 450

:label Input
:number 0

:label Timeout
:number 0

This is where our program starts.
:label Program
Set up interrupt vector.
:copy $[Interrupt_Vector] #[Pointer]
:add #[Pointer] ${screen} #[Pointer]
:copy $[Screen] @[Pointer]
:copy $[Interrupt_Vector] #[Pointer]
:add #[Pointer] ${input} #[Pointer]
:copy $[Input] @[Pointer]
:copy $[Interrupt_Vector] #[Pointer]
:add #[Pointer] ${timeout} #[Pointer]
:copy $[Timeout] @[Pointer]

Your program goes here.
:label Start
:copy $[Hello] #[DS.Pointer]
:jsub $[Draw_String]
:halt

Put the global variables and lists here.
:label Pointer
:number 0
:label Hello
:string "Hello world!"

Place subroutine variables here.
:label DS.Pointer
:number 0
:label DS.Count
:number 0
:label DS.Index
:number 0
:label DS.Screen_Pos
:number 0

:label Draw_String
Get string size.
:copy @[DS.Pointer] #[DS.Count]
:copy $0 #[DS.Index]
:copy $[Screen] #[DS.Screen_Pos]
:label DS.Loop
:test #[DS.Index] = #[DS.Count] [DS.End] {take-no-jump}
:add #[DS.Pointer] $1 #[DS.Pointer]
Copy character to screen.
:copy @[DS.Pointer] @[DS.Screen_Pos]
:add #[DS.Index] $1 #[DS.Index]
:add #[DS.Screen_Pos] $1 #[DS.Screen_Pos]
:jump [DS.Loop]
:label DS.End
:interrupt {screen}
:return
