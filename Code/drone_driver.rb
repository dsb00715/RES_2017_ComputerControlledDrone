require 'rs_232'
include Rs232
require 'io/console'

p "**************************"
p "Use A - W - S - D  keys to fly drone."
p "**************************" 

p "Conecting..."

available = Dir.entries('/dev/').select {|a| a.include?('ACM')}.first

port = new_serial_port("/dev/#{available}", baud_rate: 115200)
 
port.connect 

p "Conection ready..."
thr = 0
ail = 0
elv = 0
rud = 0

while(1) do
  command = STDIN.getch.downcase  
  p command 
  port.write command.downcase 
  
  if %w(a d q e).include?  command
    sleep(0.300) 
    port.write(command == 'a' ? 'd' : 'a') if command == 'a' || command == 'd' 
    port.write(command == 'q' ? 'e' : 'q') if command == 'q' || command == 'e'       
  end   
      
  if command == "\u0003"
    p "Closing Connection"
    port.close 
    p "Connection Closed"   
    exit 
  end
  prev_cmd = command 
end 
