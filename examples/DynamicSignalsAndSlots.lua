function foo() 
  print( "Hello") 
end

d = qt.dynamic()

function d.slot_in_qt()
  print("Slot in qt")
end

function d.slot_in_qt()
  print("Slot in qt2")
end


qt.connect (d, "some_signal()", d, "slot_in_qt()")
d:some_signal()
