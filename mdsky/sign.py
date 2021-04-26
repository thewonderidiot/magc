from PySide2.QtWidgets import QWidget
from PySide2.QtGui import QPainter

class Sign(QWidget):
    def __init__(self, parent, el_pix):
        super().__init__(parent)
        self._minus = 0
        self._plus = 0

        self._el_pix = el_pix

        self.value = b' '

        self._setup_ui()

    def _setup_ui(self):
        self.setFixedSize(20,26)

    def set_minus_bit(self, bit):
        if self._minus != bit:
            self._minus = bit
            self.update_display()

    def set_plus_bit(self, bit):
        if self._plus != bit:
            self._plus = bit
            self.update_display()

    def update_display(self):
        if self._plus:
            self.value = b'+'
        elif self._minus:
            self.value = b'-'
        else:
            self.value = b' '

        self.update()

    def paintEvent(self, event):
        p = QPainter(self)
        if self._minus | self._plus:
            p.drawPixmap(0, 10, self._el_pix, 143, 62, 20, 6)
        if self._plus:
            p.drawPixmap(8,  0, self._el_pix, 156, 50, 5, 11)
            p.drawPixmap(8, 15, self._el_pix, 156, 50, 5, 11)
