#include "Gizmos.h"
#include <sstream>
#include <glad/glad.h>
#include <glm/ext.hpp>

Gizmos* Gizmos::sm_singleton = nullptr;

Gizmos::Gizmos(unsigned int a_maxLines, unsigned int a_maxTris)
	: m_maxLines(a_maxLines),
	m_maxTris(a_maxTris),
	m_lineCount(0),
	m_triCount(0),
	m_lines(new GizmoLine[a_maxLines]),
	m_tris(new GizmoTri[a_maxTris])
{
	//\==============================================================================================
	//\ Create our Vertex Shader from a char array
	const char* vsSource = "#version 150\n \
					 in vec4 Position; \
					 in vec4 Colour; \
					 out vec4 vColour; \
					 uniform mat4 ProjectionView; \
					 void main() { vColour = Colour; gl_Position = ProjectionView * Position; }";

	m_vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(m_vertexShader, 1, (const char**)&vsSource, 0);
	glCompileShader(m_vertexShader);
	//Test our shader for succesful compiliation
	
	const char* fsSource = "#version 150\n \
					 in vec4 vColour; \
					 out vec4 FragColor; \
					 void main()	{ FragColor = vColour; }";

	m_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(m_fragmentShader, 1, (const char**)&fsSource, 0);
	glCompileShader(m_fragmentShader);
	

	m_programID = glCreateProgram();
	glAttachShader(m_programID, m_vertexShader);
	glAttachShader(m_programID, m_fragmentShader);
	glBindAttribLocation(m_programID, 0, "Position");
	glBindAttribLocation(m_programID, 1, "Colour");
	glLinkProgram(m_programID);
	
	// create VBOs
	glGenBuffers(1, &m_lineVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
	glBufferData(GL_ARRAY_BUFFER, m_maxLines * sizeof(GizmoLine), m_lines, GL_DYNAMIC_DRAW);


	glGenBuffers(1, &m_triVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_triVBO);
	glBufferData(GL_ARRAY_BUFFER, m_maxTris * sizeof(GizmoTri), m_tris, GL_DYNAMIC_DRAW);


	glGenVertexArrays(1, &m_lineVAO);
	glBindVertexArray(m_lineVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GizmoVertex), 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_TRUE, sizeof(GizmoVertex), ((char*)0) + 16);

	glGenVertexArrays(1, &m_triVAO);
	glBindVertexArray(m_triVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_triVBO);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GizmoVertex), 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_TRUE, sizeof(GizmoVertex), ((char*)0) + 16);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Gizmos::~Gizmos()
{
	delete[] m_lines;
	delete[] m_tris;
	glDeleteBuffers(1, &m_lineVBO);
	glDeleteBuffers(1, &m_triVBO);
	glDeleteVertexArrays(1, &m_lineVAO);
	glDeleteVertexArrays(1, &m_triVAO);
	glDeleteProgram(m_programID);
	glDeleteShader(m_fragmentShader);
	glDeleteShader(m_vertexShader);
}

void Gizmos::create(unsigned int a_maxLines /* = 16384 */, unsigned int a_maxTris /* = 16384 */)
{
	if (sm_singleton == nullptr)
		sm_singleton = new Gizmos(a_maxLines, a_maxTris);
}

void Gizmos::destroy()
{
	delete sm_singleton;
	sm_singleton = nullptr;
}

void Gizmos::clear()
{
	sm_singleton->m_lineCount = 0;
	sm_singleton->m_triCount = 0;
}

// Adds 3 unit-length lines (red,green,blue) representing the 3 axis of a transform, 
// at the transform's translation. Optional scale available.
void Gizmos::addTransform(const glm::mat4& a_transform, float a_fScale /* = 1.0f */)
{
	glm::vec4 vXAxis = a_transform[3] + a_transform[0] * a_fScale;
	glm::vec4 vYAxis = a_transform[3] + a_transform[1] * a_fScale;
	glm::vec4 vZAxis = a_transform[3] + a_transform[2] * a_fScale;

	glm::vec4 vRed(1, 0, 0, 1);
	glm::vec4 vGreen(0, 1, 0, 1);
	glm::vec4 vBlue(0, 0, 1, 1);

	addLine(a_transform[3].xyz, vXAxis.xyz, vRed, vRed);
	addLine(a_transform[3].xyz, vYAxis.xyz, vGreen, vGreen);
	addLine(a_transform[3].xyz, vZAxis.xyz, vBlue, vBlue);
}



void Gizmos::addBox(const glm::vec3& a_center,
	const glm::vec3& a_vDimensions,
	const bool& a_filled,
	const glm::vec4& a_fillColour /*= glm::vec4( 1.f, 1.f, 1.f, 1.f)*/,
	const glm::mat4& a_transform /* = mat4::identity */,
	glm::vec3** vertexData /*= nullptr*/, unsigned int* a_vertexCount /*= nullptr*/)
{

	glm::vec3 vX(a_vDimensions.x * 0.5f, 0, 0);
	glm::vec3 vY(0, a_vDimensions.y * 0.5f, 0);
	glm::vec3 vZ(0, 0, a_vDimensions.z * 0.5f);

	vX = (a_transform * glm::vec4(vX, 0)).xyz;
	vY = (a_transform * glm::vec4(vY, 0)).xyz;
	vZ = (a_transform * glm::vec4(vZ, 0)).xyz;


	glm::vec3 vVerts[8];

	// top verts
	vVerts[0] = a_center - vX - vZ - vY;
	vVerts[1] = a_center - vX + vZ - vY;
	vVerts[2] = a_center + vX + vZ - vY;
	vVerts[3] = a_center + vX - vZ - vY;

	// bottom verts
	vVerts[4] = a_center - vX - vZ + vY;
	vVerts[5] = a_center - vX + vZ + vY;
	vVerts[6] = a_center + vX + vZ + vY;
	vVerts[7] = a_center + vX - vZ + vY;

	if (vertexData != nullptr)
	{
		*vertexData = new glm::vec3[8];
		memcpy(*vertexData, vVerts, sizeof(glm::vec3) * 8);
		*a_vertexCount = 8;
	}

	glm::vec4 vWhite(1, 1, 1, 1);

	addLine(vVerts[0], vVerts[1], vWhite, vWhite);
	addLine(vVerts[1], vVerts[2], vWhite, vWhite);
	addLine(vVerts[2], vVerts[3], vWhite, vWhite);
	addLine(vVerts[3], vVerts[0], vWhite, vWhite);

	addLine(vVerts[4], vVerts[5], vWhite, vWhite);
	addLine(vVerts[5], vVerts[6], vWhite, vWhite);
	addLine(vVerts[6], vVerts[7], vWhite, vWhite);
	addLine(vVerts[7], vVerts[4], vWhite, vWhite);

	addLine(vVerts[0], vVerts[4], vWhite, vWhite);
	addLine(vVerts[1], vVerts[5], vWhite, vWhite);
	addLine(vVerts[2], vVerts[6], vWhite, vWhite);
	addLine(vVerts[3], vVerts[7], vWhite, vWhite);

	if (a_filled)
	{
		// top
		addTri(vVerts[2], vVerts[1], vVerts[0], a_fillColour);
		addTri(vVerts[3], vVerts[2], vVerts[0], a_fillColour);

		// bottom
		addTri(vVerts[5], vVerts[6], vVerts[4], a_fillColour);
		addTri(vVerts[6], vVerts[7], vVerts[4], a_fillColour);

		// front
		addTri(vVerts[4], vVerts[3], vVerts[0], a_fillColour);
		addTri(vVerts[7], vVerts[3], vVerts[4], a_fillColour);

		// back
		addTri(vVerts[1], vVerts[2], vVerts[5], a_fillColour);
		addTri(vVerts[2], vVerts[6], vVerts[5], a_fillColour);

		// left
		addTri(vVerts[0], vVerts[1], vVerts[4], a_fillColour);
		addTri(vVerts[1], vVerts[5], vVerts[4], a_fillColour);

		// right
		addTri(vVerts[2], vVerts[3], vVerts[7], a_fillColour);
		addTri(vVerts[6], vVerts[2], vVerts[7], a_fillColour);
	}
}

void Gizmos::addCylinder(const glm::vec3& a_center, float a_radius,
	float a_fHalfLength, unsigned int a_segments,
	const bool& a_filled, const glm::vec4& a_fillColour /*= glm::vec4(1.f, 1.f, 1.f, 1.f)*/,
	const glm::mat4& a_transform /* = mat4::identity */,
	glm::vec3** vertexData /*= nullptr*/, unsigned int* a_vertexCount /*= nullptr*/)
{
	glm::vec4 vWhite(1, 1, 1, 1);

	float fSegmentSize = (2 * glm::pi<float>()) / a_segments;
	if (vertexData != nullptr)
	{
		*vertexData = new glm::vec3[a_segments * 12];
		*a_vertexCount = a_segments * 12;
	}


	for (unsigned int i = 0; i < a_segments; ++i)
	{
		glm::vec3 v0top(0, a_fHalfLength, 0);
		glm::vec3 v1top(sinf(i * fSegmentSize) * a_radius, a_fHalfLength, cosf(i * fSegmentSize) * a_radius);
		glm::vec3 v2top(sinf((i + 1) * fSegmentSize) * a_radius, a_fHalfLength, cosf((i + 1) * fSegmentSize) * a_radius);
		glm::vec3 v0bottom(0, -a_fHalfLength, 0);
		glm::vec3 v1bottom(sinf(i * fSegmentSize) * a_radius, -a_fHalfLength, cosf(i * fSegmentSize) * a_radius);
		glm::vec3 v2bottom(sinf((i + 1) * fSegmentSize) * a_radius, -a_fHalfLength, cosf((i + 1) * fSegmentSize) * a_radius);

		v0top = (a_transform * glm::vec4(v0top, 0)).xyz;
		v1top = (a_transform * glm::vec4(v1top, 0)).xyz;
		v2top = (a_transform * glm::vec4(v2top, 0)).xyz;
		v0bottom = (a_transform * glm::vec4(v0bottom, 0)).xyz;
		v1bottom = (a_transform * glm::vec4(v1bottom, 0)).xyz;
		v2bottom = (a_transform * glm::vec4(v2bottom, 0)).xyz;


		// triangles
		if (a_filled)
		{
			addTri(a_center + v0top, a_center + v1top, a_center + v2top, a_fillColour);
			addTri(a_center + v0bottom, a_center + v2bottom, a_center + v1bottom, a_fillColour);
			addTri(a_center + v2top, a_center + v1top, a_center + v1bottom, a_fillColour);
			addTri(a_center + v1bottom, a_center + v2bottom, a_center + v2top, a_fillColour);

			if (vertexData != nullptr)
			{
				unsigned int index = i * 12;

				(*vertexData)[index] = a_center + v0top; (*vertexData)[index + 1] = a_center + v1top; (*vertexData)[index + 2] = a_center + v2top;
				index += 3;
				(*vertexData)[index] = a_center + v0bottom; (*vertexData)[index + 1] = a_center + v2bottom; (*vertexData)[index + 2] = a_center + v1bottom;
				index += 3;
				(*vertexData)[index] = a_center + v2top; (*vertexData)[index + 1] = a_center + v1top; (*vertexData)[index + 2] = a_center + v1bottom;
				index += 3;
				(*vertexData)[index] = a_center + v1bottom; (*vertexData)[index + 1] = a_center + v2bottom; (*vertexData)[index + 2] = a_center + v2top;
			}
		}
		// lines
		addLine(a_center + v1top, a_center + v2top, vWhite, vWhite);
		addLine(a_center + v1top, a_center + v1bottom, vWhite, vWhite);
		addLine(a_center + v1bottom, a_center + v2bottom, vWhite, vWhite);
	}
}

void Gizmos::addCircle(const glm::vec3& a_center, float a_radius, unsigned int a_segments,
	const bool& a_filled, const glm::vec4& a_Colour /*= glm::vec4(1.f, 1.f, 1.f, 1.f*/,
	const glm::mat4& a_transform /* = mat4::identity */,
	glm::vec3** vertexData /*= nullptr*/, unsigned int* a_vertexCount /*= nullptr*/)
{
	//calculate the inner angle for each segment
	float fAngle = (2 * glm::pi<float>()) / a_segments;
	//We can start our first edge vector at (0,0,radius) as sin(0) = 0, cos(0) = 1
	glm::vec4 v3Edge1(0, 0, a_radius, 0);
	if (vertexData != nullptr)
	{
		*vertexData = new glm::vec3[a_segments * 3];
		*a_vertexCount = a_segments * 3;
	}

	for (unsigned int i = 0; i < a_segments; ++i)
	{

		glm::vec4 v3Edge2(sinf((i + 1) * fAngle) * a_radius, 0, cosf((i + 1) * fAngle) * a_radius, 0);

		v3Edge1 = a_transform * v3Edge1;
		v3Edge2 = a_transform * v3Edge2;

		if (a_filled)
		{
			addTri(a_center, v3Edge1.xyz, a_center + v3Edge2.xyz, a_Colour);
			addTri(a_center + v3Edge2.xyz, a_center + v3Edge1.xyz, a_center, a_Colour);
			if (vertexData != nullptr)
			{
				unsigned int index = i * 3;
				(*vertexData)[index] = a_center;
				(*vertexData)[index + 1] = v3Edge1.xyz;
				(*vertexData)[index + 2] = a_center + v3Edge2.xyz;
			}
		}
		else
		{
			// line
			addLine(a_center + v3Edge1.xyz, a_center + v3Edge2.xyz, a_Colour);
		}
		v3Edge1 = v3Edge2;
	}
}

void Gizmos::addSphere(const glm::vec3& a_center, int a_rows, int a_columns, float a_radius, const glm::vec4& a_fillColour,
	const glm::mat4* a_transform /*= nullptr*/, float a_longMin /*= 0.f*/, float a_longMax /*= 360*/,
	float a_latMin /*= -90*/, float a_latMax /*= 90*/,
	glm::vec3** vertexData/* = nullptr*/, unsigned int* a_vertexCount /*= nullptr*/)
{
	//Invert these first as the multiply is slightly quicker
	float invColumns = 1.0f / float(a_columns);
	float invRows = 1.0f / float(a_rows);

	float DEG2RAD = glm::pi<float>() / 180;

	//Lets put everything in radians first
	float latitiudinalRange = (a_latMax - a_latMin) * DEG2RAD;
	float longitudinalRange = (a_longMax - a_longMin) * DEG2RAD;
	// for each row of the mesh
	glm::vec3* v4Array = new glm::vec3[a_rows*a_columns + a_columns];

	for (int row = 0; row <= a_rows; ++row)
	{
		// y ordinates this may be a little confusing but here we are navigating around the xAxis in GL
		float ratioAroundXAxis = float(row) * invRows;
		float radiansAboutXAxis = ratioAroundXAxis * latitiudinalRange + (a_latMin * DEG2RAD);
		float y = a_radius * sin(radiansAboutXAxis);
		float z = a_radius * cos(radiansAboutXAxis);

		for (int col = 0; col <= a_columns; ++col)
		{
			float ratioAroundYAxis = float(col) * invColumns;
			float theta = ratioAroundYAxis * longitudinalRange + (a_longMin * DEG2RAD);
			glm::vec3 v4Point(-z * sinf(theta), y, -z * cosf(theta));

			if (a_transform != nullptr)
			{
				v4Point = (*a_transform * glm::vec4(v4Point, 0)).xyz;
			}

			int index = row * a_columns + (col % a_columns);
			v4Array[index] = a_center + v4Point;
		}
	}

	if (vertexData != nullptr)
	{
		unsigned int vertexCount = a_rows * a_columns + a_columns;
		*vertexData = new glm::vec3[vertexCount];
		memcpy(*vertexData, v4Array, sizeof(glm::vec3)*vertexCount);
		*a_vertexCount = a_rows * a_columns + a_columns;
	}

	for (int face = 0; face < (a_rows)*(a_columns); ++face)
	{
		int iNextFace = face + 1;

		if (iNextFace % a_columns == 0)
		{
			iNextFace = iNextFace - (a_columns);
		}

		addLine(v4Array[face], v4Array[face + a_columns], glm::vec4(1.f, 1.f, 1.f, 1.f), glm::vec4(1.f, 1.f, 1.f, 1.f));

		if (face % a_columns == 0 && longitudinalRange < (glm::pi<float>() * 2))
		{
			continue;
		}
		addLine(v4Array[iNextFace + a_columns], v4Array[face + a_columns], glm::vec4(1.f, 1.f, 1.f, 1.f), glm::vec4(1.f, 1.f, 1.f, 1.f));

		addTri(v4Array[iNextFace + a_columns], v4Array[face], v4Array[iNextFace], a_fillColour);
		addTri(v4Array[iNextFace + a_columns], v4Array[face + a_columns], v4Array[face], a_fillColour);
	}

	delete[] v4Array;
}


void Gizmos::addLine(const glm::vec3& a_rv0, const glm::vec3& a_rv1, const glm::vec4& a_colour)
{
	addLine(a_rv0, a_rv1, a_colour, a_colour);
}

void Gizmos::addLine(const glm::vec3& a_rv0, const glm::vec3& a_rv1, const glm::vec4& a_colour0, const glm::vec4& a_colour1)
{
	if (sm_singleton != nullptr &&
		sm_singleton->m_lineCount < sm_singleton->m_maxLines)
	{
		sm_singleton->m_lines[sm_singleton->m_lineCount].v0.position = glm::vec4(a_rv0, 1);
		sm_singleton->m_lines[sm_singleton->m_lineCount].v0.colour = a_colour0;
		sm_singleton->m_lines[sm_singleton->m_lineCount].v1.position = glm::vec4(a_rv1, 1);
		sm_singleton->m_lines[sm_singleton->m_lineCount].v1.colour = a_colour1;

		sm_singleton->m_lineCount++;
	}
}

void Gizmos::addTri(const glm::vec3& a_rv0, const glm::vec3& a_rv1, const glm::vec3& a_rv2, const glm::vec4& a_colour)
{
	if (sm_singleton != nullptr)
	{
		if (sm_singleton->m_triCount < sm_singleton->m_maxTris)
		{
			sm_singleton->m_tris[sm_singleton->m_triCount].v0.position = glm::vec4(a_rv0, 1);
			sm_singleton->m_tris[sm_singleton->m_triCount].v1.position = glm::vec4(a_rv1, 1);
			sm_singleton->m_tris[sm_singleton->m_triCount].v2.position = glm::vec4(a_rv2, 1);
			sm_singleton->m_tris[sm_singleton->m_triCount].v0.colour = a_colour;
			sm_singleton->m_tris[sm_singleton->m_triCount].v1.colour = a_colour;
			sm_singleton->m_tris[sm_singleton->m_triCount].v2.colour = a_colour;

			sm_singleton->m_triCount++;
		}
	}
}

void Gizmos::draw(const glm::mat4& a_projectionView)
{
	if (sm_singleton != nullptr &&
		(sm_singleton->m_lineCount > 0 || sm_singleton->m_triCount > 0))
	{
		glUseProgram(sm_singleton->m_programID);

		unsigned int projectionViewUniform = glGetUniformLocation(sm_singleton->m_programID, "ProjectionView");
		glUniformMatrix4fv(projectionViewUniform, 1, false, glm::value_ptr(a_projectionView));

		if (sm_singleton->m_lineCount > 0)
		{
			glBindBuffer(GL_ARRAY_BUFFER, sm_singleton->m_lineVBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sm_singleton->m_lineCount * sizeof(GizmoLine), sm_singleton->m_lines);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(sm_singleton->m_lineVAO);
			glDrawArrays(GL_LINES, 0, sm_singleton->m_lineCount * 2);
		}

		if (sm_singleton->m_triCount > 0)
		{
			glBindBuffer(GL_ARRAY_BUFFER, sm_singleton->m_triVBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sm_singleton->m_triCount * sizeof(GizmoTri), sm_singleton->m_tris);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(sm_singleton->m_triVAO);
			glDrawArrays(GL_TRIANGLES, 0, sm_singleton->m_triCount * 3);
		}

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glUseProgram(0);

	}
}

