


int fn(int x)
{
	switch (x) {
	case -1: return 0;


	case 0:  return 2;
	case 2:  return 3;


	case 12: return 7;
	default: return 255;
	}



}

int fn2()
{


	return 7;
}



int main(int argc, char *argv[])
{


	if (argc > 1)
		return fn(argc);

	return fn2();


}
