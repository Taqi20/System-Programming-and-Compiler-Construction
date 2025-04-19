**      START   1000
**      USING   *,15
LOOP    L       R1,VAR
        A       R1,ONE
        BR      LOOP
ONE     DC      F'1'
VAR     DC      F'5'
        END     LOOP
