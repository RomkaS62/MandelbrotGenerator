package com.romkas.mandelbrot;

public class IntPoint
{
	public int x;
	public int y;

	public IntPoint(int x, int y)
	{
		this.x = x;
		this.y = y;
	}

	public int dx(IntPoint other)
	{
		return other.x - x;
	}

	public int dy(IntPoint other)
	{
		return other.y - y;
	}
}
