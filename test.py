import numpy as np
import pandas as pd

def check_status(current, time):
    
    y = -5.981861989378207 -14.858981685527775 * (current/12) + 9.291278202911599 * (time/7200)
    p = 1/(1+np.exp(-y))
        
    return p

data = pd.read_csv('C:\\Users\\janik\\Wolke\\OneDrive\\Documents\\Projekte\\waschmaschine\\validation\\Waesche_02082021.csv', index_col=None, delimiter= ';', decimal= '.')
x_1 = data['current'].to_numpy()
x_2 = data['delta'].to_numpy()

done = 0 # sobald fertig, nicht weiter machen, damit die Nachricht nicht mehrmals verschickt wird
count = 0 # wenn mehr als fünf mal p>0.5 ist, dann ist der Waschvorgang vermutlich beendet, Beispiel: Waesche_02082021
n = 0 # nur wenn fünf mal in folge

for i in range(len(x_1)):
    
    p = check_status(x_1[i], x_2[i])

    if p > 0.5 and (x_2[i] - x_2[i-1]) == 2:
        count = count + 1
    else:
        count = 0

    if count > 12:
        done = 1
        print(f'Zeile: {i+2} | Current: {x_1[i]} A | Time: {x_2[i]} Seconds | p: {round(p*100,2)} %')
        break
