N=20;

rotate (a = [0, 0, 36]) {
  union() {
    translate ([1, 0]) {
	polygon (points = [[(N - 1)*cos(180/N), -(N - 1)*sin(180/N)],
			   [(N - 3)*cos(270/N), -(N - 3)*sin(270/N)],
			   [(N - 1)*cos(270/N), -(N - 1)*sin(270/N)]]);

	polygon (points = [[(N - 1)*cos(180/N), -(N - 1)*sin(180/N)],
			   [(N - 3)*cos(180/N), -(N - 3)*sin(180/N)],
			   [(N - 3)*cos(270/N), -(N - 3)*sin(270/N)]]);

	polygon (points = [[N - 1, 0], [N - 3, 0],
			   [(N - 3)*cos(180/N), -(N - 3)*sin(180/N)]]);

	polygon (points = [[N - 1, 0],
			[(N - 3)*cos(180/N), -(N - 3)*sin(180/N)],
			[(N - 1)*cos(180/N), -(N - 1)*sin(180/N)]]);

	polygon (points = [[N - 1, 0], [N - 3, 0],
			  [(N - 3)*cos(180/N), (N - 3)*sin(180/N)]]);

	polygon (points = [[N - 1, 0],
			  [(N - 3)*cos(180/N), (N - 3)*sin(180/N)],
			  [(N - 1)*cos(180/N), (N - 1)*sin(180/N)]]);

	polygon (points = [[(N - 1)*cos(180/N), (N - 1)*sin(180/N)],
			   [(N - 3)*cos(180/N), (N - 3)*sin(180/N)],
			   [(N - 3)*cos(270/N), (N - 3)*sin(270/N)]]);

	polygon (points = [[(N - 1)*cos(180/N), (N - 1)*sin(180/N)],
			   [(N - 3)*cos(270/N), (N - 3)*sin(270/N)],
			   [(N - 1)*cos(270/N), (N - 1)*sin(270/N)]]);
    }
    circle (r = 20);
  }
}
