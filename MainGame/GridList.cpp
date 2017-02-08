#pragma once

#ifndef GRIDLIST_CPP
#define GRIDLIST_CPP

#include <cmath>
// ReSharper disable once CppUnusedIncludeDirective
#include "GridList.h"
#include <iostream>

template <class T>
GridList<T>::GridList() : width(0), height(0), size(0, 0)
{
}

template <class T>
Vector2i GridList<T>::getBlockSize() const
{
	return size;
}

template <class T>
GridList<T>::GridList(int width, int height, Vector2i size)
{
	this->width = width;
	this->height = height;
	this->size = size;
	auto vectorSize = int(ceil(double(height) / size.y) * ceil(double(width) / size.x));
	cells.resize(vectorSize);
}

template <class T>
GridList<T>::~GridList()
{
	for (std::vector<T*> cell : cells)
	{
		for (T* ptr : cell)
		{
			delete ptr;
		}
		cell.clear();
	}
	cells.clear();
}

template <class T>
int GridList<T>::getIndexByPoint(int x, int y) const
{
	auto y1 = y / size.y;
	auto x1 = ceil(double(width) / size.x);
	auto result = x1 * y1 + x / size.x;
	return int(result);
}

template <class T>
void GridList<T>::addItem(T* item, const std::string& name, int x, int y)
{
	if (items.find(name) != items.end())
		throw std::invalid_argument("The key '" + name + "' already exists in the Grid.");

	auto index = getIndexByPoint(x, y);
	auto position = std::make_pair(index, int(cells[index].size()));
	cells[index].push_back(item);
	items.insert({ name, position });
}

template <class T>
T* GridList<T>::getItemByName(const std::string& name)
{
	auto position = items.at(name);
	return cells[position.first][position.second];
}

template <class T>
std::vector<T*> GridList<T>::getItems(int upperLeftX, int upperLeftY, int bottomRightX, int bottomRightY, int width)
{
	if (upperLeftX < 0)
		//upperLeftX = 0;
		upperLeftX = width - abs(upperLeftX);
	if (upperLeftY < 0)
		upperLeftY = 0;
	if (bottomRightX > width)
		//bottomRightX = width;
		bottomRightX = bottomRightX % width;
	if (bottomRightY > height)
		bottomRightY = height;

	std::vector<T*> result;

	auto rowsCount = int(ceil(double(bottomRightY - upperLeftY) / size.y));
	auto firstColumn = getIndexByPoint(upperLeftX, upperLeftY);
	auto lastColumn = getIndexByPoint(bottomRightX, upperLeftY);
	auto columnsPerRow = int(ceil(double(width) / size.x));
	auto maxColumn = int(cells.size()) - 1;

	for (auto i = 0; i <= rowsCount; i++)
	{
		
		if (lastColumn >= maxColumn)
			lastColumn = maxColumn;
		if (firstColumn <= lastColumn)
		{
			for (int j = firstColumn; j <= lastColumn; j++)
			{
				for (auto k = 0; k < cells[j].size(); k++)
				{
					result.push_back(cells[j][k]);
				}
			}
		}
		else
		{
			int j = firstColumn;
			while (j != lastColumn)
			{
				if (j % columnsPerRow == 0)
					j -= columnsPerRow;
				for (auto k = 0; k < cells[j].size(); k++)
				{
					result.push_back(cells[j][k]);
				}
				j++;
			}
		}

		firstColumn += columnsPerRow;
		lastColumn += columnsPerRow;
		
	}
	return result;
}

template <class T>
void GridList<T>::updateItemPosition(const std::string& name, int x, int y)
{
	auto position = items.at(name);
	auto item = cells[position.first][position.second];
	cells[position.first].erase(cells[position.first].begin() + position.second);

	auto index = getIndexByPoint(x, y);
	position = std::make_pair(index, int(cells[index].size()));
	cells[index].push_back(item);
	items[name] = position;
}

#endif
