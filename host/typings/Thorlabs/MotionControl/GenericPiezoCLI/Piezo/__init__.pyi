from typing import ClassVar, Self

from System import Enum

class PiezoControlModeTypes(Enum):
    OpenLoop: ClassVar[Self]
    CloseLoop: ClassVar[Self]
    OpenLoopSmooth: ClassVar[Self]
    CloseLoopSmooth: ClassVar[Self]
