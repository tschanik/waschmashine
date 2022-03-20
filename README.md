# Einleitung
Da sich die Waschmaschine im Keller befindet, ist der Signalton, welches das Beenden eines Waschvorgangs signalisiert, nicht immer zu hören. Daher ist das Ziel dieses Projekt, eine Nachricht auf Telegram zu bekommen, sobald der Waschvorgang beendet ist. Desweiteren soll eine Übersicht der verbrauchten Energie, sowie die benötigte Zeit als Chatnachricht auf Telegram übermittelt werden. 

Zusätzlich soll der aktuelle Verbrauch, sowie weitere Daten, während des Waschvorgangs, über eine lokale Website jederzeit einsehbar sein.

Für eine spätere Weiterentwicklung, in Form eines Dashboards oder ähnliches, sollen die aufgenommenen Daten in eine Datenbank gespeichert werden. Diese Daten wurden auch verwendet, um herauszufinden, wann ein Waschvorgang beendet ist. Dazu wurde zunächst mit Machine Learning versucht, ein Modell zu trainieren, welches das Muster in den letzten Sekunden eines Waschvorganges erkennt (siehe Kapitel Verläufe). Aufgrund der zu vieler false positives bei der Verwendung des Machine Learning-Modells, wurde aber in der finalen Version nur die minimale Anzahl an Messpunkten im Idle genutzt, um das fertigstellen eines Waschvorganges zu ermitteln.

# Verwendete Hardware

Nachfolgend eine Liste der für dieses Projekt verwendete Hardware:

* Mikrocontroller ESP32
* Wirbelstromsensor YHDC SCT013 30A/1V
* Analog-Digitalwandler ADS1115

Da es genug Anleitungen für die Verwendung der Hardware gibt, soll hier zunächst nur auf die Umsetzung der Software eingegangen werden.

# Projektstruktur

test
    
    Beinhaltet die Stromverläufe als .csv-Dateien, um das Machine Learning-Modell zu testen.

training

     Beinhaltet die Stromverläufe als .csv-Dateien, um das Machine Learning-Modell zu trainieren.

Die verwendeten Einheiten sind in nachfolgender Tabelle aufgelistet: 

| Spalte  | Einheit |
|---------|---------|
| Current | Ampere  | 
| Delta   | Sekunde |
| Status  | -       |


Kalibrierung.ino

    Es erfolgte zunächst mit diversen Haushaltsgeräten eine Kalibrierung, da die Angabe von 30A/1V des verwendeten Sensors nicht genau genug ist. Die Ergebisse der Kalibrierung sind in der Excel-Datei Kalibrierung.xlsx gespeichert.

index.html
    
    Beinhaltet die HTML-Formatierung der Website um den aktuellen Status, während eines Waschvorganges, einzusehen. Diese Datei dient nur der besseren Übersicht. Die finale Version ist in der Datei waschmachine.ino implementiert.

ml.ipynb
    
    Jupyter Notebook für das trainieren eines Machine Learning-Modells.

test.py

    Eine Python-Datei, um die erlernte Funktion zu testen.

waschmachine.ino

    C++-Datei für den Mikrocontroller ESP32.
    


# Verläufe

Betrachtet man die letzten Messpunkte von mehreren diversen Waschprogramme (alle zwei Sekunden wird der aktuelle Stromverbrauch gemessen), fällt ein gewisses Muster auf (siehe Abbildung 1):

![Stromverläufe der letzten Sekunden](https://raw.githubusercontent.com/tschanik/waschmashine/main/plot.JPG "Abbildung 1")
Abbildung 1

Hierbei ist zu erkennen, dass sich die Trommel noch viermal dreht, bevor nach der vierten Umdrehung die Türe entriegelt wird(Siehe Messpunkt 36). Ziel ist es, die Nachricht über die Beendigung des Waschvorganges so nah wie möglich an diesem Punkt zu versenden. Dabei wurde zunächst mit Machine Learning versucht, eine Funktion zu finden, die dieses Verlauf erkennt.

# Machine Learning

Die erlernte Funktion der logistischen Regression, in der Datei ml.ipynb, ist nachfolgend zu sehen (siehe Datei test.py):

```python
y = -5.981861989378207 -14.858981685527775 * (current/12) + 9.291278202911599 * (time/7200)

p = 1/(1+np.exp(-y))
```

Dabei wurden zunächst die Input-Variablen (Stromstärke und die Zeit) normalisiert. Da die Waschmaschine nie mehr als 12 Ampere verbraucht und das längste Waschproramm 7200 Sekunden benötigte, wurden diese Werte als Maximalwerte verwendet. Das Ende eines Waschprogrammes ist erreicht, sobald die Wahrscheinlichkeit p bei 50% oder mehr liegt.

Da es auch Waschprogramme gibt, bei denen die Waschmaschine relativ lange im Idle ist bevor das Waschprogramm fortgesetzt wird, kommt es bei solchen Programmen häufig zu false positive Meldungen. Dies wurde zunächst versucht mit einer Anpassung des Threshold zu umgehen (p>0.8 anstatt p>=0.5), was aber zu einer zu langen Wartezeit bei normalen Waschprogrammen führte.


# Alternativ Methode

Dazu wurden alle aufgezeichneten Waschvorgänge eingelesen um die maximale Anzahl an Messpunkten im Idle, während eines Waschvorganges zu ermitteln.

```python
counter = 0
max = 0
for i in range(1,len(x)):
    if (x[i,1] - x[i-1,1] == 2) & (x[i,1] > 18*60):
        counter = counter + 1
        if counter > max:
            max = counter
    else:
        counter = 0

print(max)

Output: 30
```

Sobald es mehr als 30 Messpunkte im Idle gibt, nachdem die ersten 18 Minuten verstrichen sind, ist das Waschprogramm beendet.


# Ergebnis

Durch die Vielzahl an Waschprogrammen, hat das Machine Learning-Modell nicht zu dem gewünschten Ergebnis geführt, das Muster am Ende eines Waschvorganges zu erkennen. Zwar konnte relativ genau das Ende eines Waschvorgangs bestimmt werden, allerdings ist die Waschmaschine auch während eines Waschvorgangs ab und zu im Idle, was zu vielen false positive Meldungen führte. Daher wurde in der finalen Version nur die minimale Anzahl an Messpunkten im Idle verwendet, um das Ende eines Waschvorganges zu ermitteln. Diese Methode hat letztendlich zu dem gewünschten Ergebnis geführt, die Nachricht über die Fertigstellung eines Waschvorganges nächstmöglich am letzten Messpunkt eines Waschprogrammes zu versenden. Zwar kommt die Nachricht mit einer Verzögerung von mehr als einer Minute (30*2 Sekunden, 2 Sekunden ist die Samplingrate), aber mit dieser Methode kommt es nicht zu falschen Meldungen während ein Waschprogramm noch läuft und sich nur im Idle befindet, bevor der Waschvorgang fortgesetzt wird.

Zudem wurde bei diesem Projekt ersichtlich, wie wichtig es ist für ein Machine Learning-Modell die Werte zu normalisieren. Da gerade die Sekunden mit zunehmender Waschdauer viel größer als der Stromverbrauch werden, kommt es hier ohne eine Normalisierung zur einer falschen Übergewichtung der Zeit.