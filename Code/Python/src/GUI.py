import serial
import time
import matplotlib.pyplot as plt
import matplotlib.animation as animation

ser = serial.Serial('COM3', baudrate = 115200, timeout=2)
time.sleep(5)
num_SEMG_connected = 2

fig = plt.figure()                                      # Create Matplotlib plots fig is the 'higher level' plot window
ax = fig.add_subplot(111)                               # Add subplot to main fig window
dataList = []                                           # Create empty list variable for later use

def getValues():
    line = ser.readline().strip()
    values = line.decode('ascii').split(',')

    return values

def animate(i, dataList, ser):                           # Transmit the char 'g' to receive the Arduino data point
    values = getValues()
    try:
        register, semg_1, semg_2 = [int(s) for s in values]
        print(semg_1)
        print(semg_2)

        GPIO_1 = (bin(register))[12]
        GPIO_2 = (bin(register))[11]
        dataList.append(semg_2) 
    except:
        pass
    
    dataList = dataList[-50:]                           # Fix the list size so that the animation plot 'window' is x number of points
    
    ax.clear()                                          # Clear last data frame
    ax.plot(dataList)                                   # Plot new data frame
    
    ax.set_ylim([-9000000, 9000000])                              # Set Y axis limit of plot
    ax.set_title("ADS1292R Moduel 1")                        # Set title of figure
    ax.set_ylabel("Value")                              # Set title of y axis 

while (1):
    
    

                                                        # Matplotlib Animation Fuction that takes takes care of real time plot.
                                                        # Note that 'fargs' parameter is where we pass in our dataList and Serial object. 
    ani = animation.FuncAnimation(fig, animate, frames=100, fargs=(dataList, ser), interval=100) 

    plt.show()                                              # Keep Matplotlib plot persistent on screen until it is closed
    ser.close()    
    