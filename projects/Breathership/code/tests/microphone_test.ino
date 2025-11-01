//Trying microphone
  breath = analogRead(A0);
  Serial.println("current read");
  Serial.println(breath);

  int mn = 1024;
  int mx = 0;

  for (int i = 0; i < 100; ++i) {
    int val = analogRead(A0);
    mn = min(mn, val);
    mx = max(mx, val);
  }

  int delta = mx - mn;

  Serial.println("delta is");
  Serial.println(delta);

  delay(5000);
